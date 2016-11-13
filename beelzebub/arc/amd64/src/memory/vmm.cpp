/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#include <memory/vmm.hpp>
#include <memory/vmm.arc.hpp>
#include <memory/pmm.hpp>
#include <memory/pmm.arc.hpp>
#include <memory/object_allocator_pools_heap.hpp>
#include <synchronization/spinlock_uninterruptible.hpp>
#include <system/cpu.hpp>
#include <system/interrupts.hpp>
#include <kernel.hpp>

#include <string.h>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

/****************
    Utilities
****************/

template<typename TInt>
static __forceinline bool Is4KiBAligned(TInt val) { return (val & (     PageSize - 1)) == 0; }

template<typename TInt>
static __forceinline bool Is2MiBAligned(TInt val) { return (val & (LargePageSize - 1)) == 0; }

/****************
    Vmm class
****************/

/*  State Machine and Configuration  */

Atomic<bool> KernelHeapOverflown {false};

Spinlock<> VmmArc::KernelHeapLock;

bool VmmArc::Page1GB = false;
bool VmmArc::VmmArc::NX = false;

inline void Alienate(Process * proc)
{
    Pml4 & pml4 = *(VmmArc::GetLocalPml4());

    pml4[VmmArc::AlienFractalIndex] = Pml4Entry(proc->PagingTable, true, true, false, VmmArc::NX);
}

/*  Statics  */

Atomic<vaddr_t> Vmm::KernelHeapCursor {VmmArc::KernelHeapStart};

vaddr_t Vmm::UserlandStart = 1ULL << 21;    //  2 MiB
vaddr_t Vmm::UserlandEnd = VmmArc::LowerHalfEnd;
vaddr_t Vmm::KernelStart = VmmArc::KernelHeapStart;
vaddr_t Vmm::KernelEnd = VmmArc::KernelHeapEnd;

/*  Initialization  */

Handle Vmm::Bootstrap(Process * const bootstrapProc)
{
    //  VmmArc::NX and VmmArc::Page1GB are set before executing this.

    if (VmmArc::NX)
        Cpu::EnableNxBit();

    paddr_t const pml4_paddr = bootstrapProc->PagingTable;

    Pml4 & newPml4 = *((Pml4 *)pml4_paddr);
    //  Cheap.

    Cr3 cr3 = Cpu::GetCr3();
    Pml4 & currentPml4 = *(cr3.GetPml4Ptr());

    for (uint16_t i = 0; i < 256; ++i)
        newPml4[i] = currentPml4[i];
    //  Temporarily-preserved identity mapping.
    //  The first cloning will discard it.

    newPml4[(uint16_t)511] = currentPml4[(uint16_t)511];

    for (uint16_t i = 256; i < VmmArc::AlienFractalIndex; ++i)
    {
        paddr_t pml3_paddr = Pmm::AllocateFrame(1);

        if unlikely(pml3_paddr == nullpaddr)
            return HandleResult::OutOfMemory;

        memset((void *)pml3_paddr, 0, PageSize);
        //  Clear again.

        newPml4[i] = Pml4Entry(pml3_paddr, true, true, true, false);
    }

    newPml4[VmmArc::LocalFractalIndex] = Pml4Entry(pml4_paddr, true, true, false, VmmArc::NX);
    newPml4[VmmArc::AlienFractalIndex] = newPml4[VmmArc::LocalFractalIndex];
    //  Very important note: the user-accessible bit is cleared.
    //  This means userland code will not be able to look at the fractal mapping.

    Vmm::Switch(nullptr, bootstrapProc);
    //  Activate, so pages can be mapped.

    //  Remapping PAS control structures.

    FrameAllocationSpace * cur = PmmArc::MainAllocator->FirstSpace;
    bool pendingLinksMapping = true;
    vaddr_t curLoc = KernelHeapCursor; //  Used for serial allocation.
    Handle res; //  Temporary result.

    do
    {
        if ((vaddr_t)cur < VmmArc::HigherHalfStart && pendingLinksMapping)
        {
            res = Vmm::MapPage(bootstrapProc, curLoc, RoundDown((paddr_t)cur, PageSize)
                , MemoryFlags::Global | MemoryFlags::Writable
                , MemoryMapOptions::NoLocking | MemoryMapOptions::NoReferenceCounting);
            //  Global because it's shared by processes, and writable for hotplug.

            ASSERT(res.IsOkayResult()
                , "Failed to map links between allocation spaces: %H"
                , res);
            //  Failure is fatal.

            PmmArc::Remap(PmmArc::MainAllocator, RoundDown((vaddr_t)cur, PageSize), curLoc);
            //  Do the actual remapping.

            pendingLinksMapping = false;
            //  One page is the maximum.

            curLoc += PageSize;
            //  Increment the current location.
        }

        paddr_t const pasStart = cur->GetMemoryStart();
        size_t controlStructuresSize = RoundUp(cur->GetControlAreaSize(), PageSize);
        //  Size of control pages.

        // if (curLoc + controlStructuresSize > VmmArc::KernelHeapEnd)
        //     break;
        // //  Well, the maximum is reached! Like this will ever happen...

        // for (size_t offset = 0; offset < controlStructuresSize; offset += PageSize)
        // {
        //     res = Vmm::MapPage(bootstrapProc, curLoc + offset, pasStart + offset
        //         , MemoryFlags::Global | MemoryFlags::Writable
        //         , MemoryMapOptions::NoLocking | MemoryMapOptions::NoReferenceCounting);

        //     ASSERT(res.IsOkayResult()
        //         , "Failed to map page #%u8 (%Xp to %XP): %H"
        //         , offset / PageSize
        //         , curLoc + offset
        //         , pasStart + offset
        //         , res);
        //     //  Failure is fatal.
        // }

        res = Vmm::MapRange(bootstrapProc
            , curLoc, pasStart, RoundUp(controlStructuresSize, PageSize)
            , MemoryFlags::Global | MemoryFlags::Writable
            , MemoryMapOptions::NoLocking | MemoryMapOptions::NoReferenceCounting);

        ASSERT(res.IsOkayResult()
            , "Failed to map range %Xp to %XP (%Xs bytes): %H"
            , curLoc
            , pasStart
            , controlStructuresSize
            , res);
        //  Failure is fatal.

        withLock (cur->LargeLocker)
            cur->Map = reinterpret_cast<LargeFrameDescriptor *>(curLoc);

        curLoc += controlStructuresSize;

        for (size_t i = 0; i < cur->GetLargeFrameCount(); ++i, curLoc += PageSize)
        {
            LargeFrameDescriptor * lDesc = cur->Map + i;

            res = Vmm::MapPage(bootstrapProc, curLoc
                , reinterpret_cast<paddr_t>(lDesc->SubDescriptors)
                , MemoryFlags::Global | MemoryFlags::Writable
                , MemoryMapOptions::NoLocking | MemoryMapOptions::NoReferenceCounting);

            ASSERT(res.IsOkayResult()
                , "Failed to map split frame subdescriptor page #%u8 (%Xp to %XP): %H"
                , i
                , curLoc
                , lDesc->SubDescriptors
                , res);

            withLock (cur->SplitLocker)
                lDesc->SubDescriptors = reinterpret_cast<SmallFrameDescriptor *>(curLoc);
        }

    } while ((cur = cur->Next) != nullptr);

    KernelHeapCursor = curLoc;

    Pml4 & pml4 = *(VmmArc::GetLocalPml4());

    for (uint16_t i = 0; i < 256; ++i)
        pml4[i] = Pml4Entry();
    //  Getting rid of those naughty identity maps.

    Vmm::Switch(nullptr, bootstrapProc);
    //  Re-activate, to flush the identity maps.

    return HandleResult::Okay;
}

Handle Vmm::Initialize(Process * proc)
{
    CpuInstructions::InvalidateTlb(VmmArc::GetAlienPml4());

    paddr_t const pml4_paddr = proc->PagingTable
        = Pmm::AllocateFrame(1, AddressMagnitude::_32bit);

    if (pml4_paddr == nullpaddr)
        return HandleResult::OutOfMemory;
    //  Do the good deed.

    Spinlock<> * alienLock = nullptr;

    InterruptGuard<> intGuard;
    //  Guard the rest of the scope from interrupts.

    if (CpuDataSetUp)
        alienLock = &(Cpu::GetProcess()->AlienPagingTablesLock);

    {   //  Lock-guarded.
        LockGuardFlexible<Spinlock<> > pml4Lg {alienLock};

        Alienate(proc);
        //  So it can be accessible.

        Pml4 & pml4Local = *(VmmArc::GetLocalPml4());
        Pml4 & pml4Alien = *(VmmArc::GetAlienPml4());

        for (uint16_t i = 0; i < 256; ++i)
            pml4Alien[i] = Pml4Entry();
        //  Userland space will be empty.

        for (uint16_t i = 256; i < VmmArc::AlienFractalIndex; ++i)
            pml4Alien[i] = pml4Local[i];
        //  Kernel-specific tables.

        pml4Alien[VmmArc::LocalFractalIndex] = Pml4Entry(pml4_paddr, true, true, false, VmmArc::NX);

        pml4Alien[511] = pml4Local[511];
        //  Last page, where the kernel and bootloader-provided shenanigans sit
        //  snuggly together and drink hot cocoa.

        if (CpuDataSetUp)
            Cpu::GetData()->LastAlienPml4 = pml4_paddr;
    }

    return proc->Vas.Initialize(UserlandStart, UserlandEnd
        , &AcquirePoolInKernelHeap, &EnlargePoolInKernelHeap, &ReleasePoolFromKernelHeap);
}

/*  Activation and Status  */

Handle Vmm::Switch(Process * const oldProc, Process * const newProc)
{
    Cr3 const newVal = Cr3(newProc->PagingTable, false, false);

    Cpu::SetCr3(newVal);

    return HandleResult::Okay;
}

bool Vmm::IsActive(Process * proc)
{
    Pml4 & pml4 = *(VmmArc::GetLocalPml4());

    return pml4[VmmArc::LocalFractalIndex].GetAddress() == proc->PagingTable;
}

bool Vmm::IsAlien(Process * proc)
{
    Pml4 & pml4 = *(VmmArc::GetLocalPml4());

    return pml4[VmmArc::AlienFractalIndex].GetAddress() == proc->PagingTable;
}

/*  Page Management  */

template<typename cbk_t>
static __hot inline Handle TranslateInternal(Process * proc
    , uintptr_t const vaddr
    , cbk_t cbk
    , bool const lockHeap
    , bool const lockAlien
    , bool const nonLocal)
{
    Pml4 * pml4p; Pml3 * pml3p; Pml2 * pml2p; Pml1 * pml1p;

    Spinlock<> * alienLock = nullptr, * heapLock = nullptr;

    if (lockAlien && nonLocal && CpuDataSetUp)
        alienLock = &(Cpu::GetProcess()->AlienPagingTablesLock);

    LockGuardFlexible<Spinlock<> > pml4Lg {alienLock};

    if (nonLocal)
    {
        Alienate(proc);

        pml4p = VmmArc::GetAlienPml4();
        pml3p = VmmArc::GetAlienPml3(vaddr);
        pml2p = VmmArc::GetAlienPml2(vaddr);
        pml1p = VmmArc::GetAlienPml1(vaddr);

        if (!CpuDataSetUp || proc->PagingTable != Cpu::GetData()->LastAlienPml4)
        {
            CpuInstructions::InvalidateTlb(pml4p);
            CpuInstructions::InvalidateTlb(pml3p);
            CpuInstructions::InvalidateTlb(pml2p);
            CpuInstructions::InvalidateTlb(pml1p);

            //  Invalidate all!

            if (CpuDataSetUp)
                Cpu::GetData()->LastAlienPml4 = proc->PagingTable;
        }
    }
    else
    {
        pml4p = VmmArc::GetLocalPml4();
        pml3p = VmmArc::GetLocalPml3(vaddr);
        pml2p = VmmArc::GetLocalPml2(vaddr);
        pml1p = VmmArc::GetLocalPml1(vaddr);
    }

    if (lockHeap)
        heapLock = vaddr < VmmArc::LowerHalfEnd ? &proc->LocalTablesLock : &VmmArc::KernelHeapLock;

    LockGuardFlexible<Spinlock<> > heapLg {heapLock};

    if unlikely(!pml4p->operator[](VmmArc::GetPml4Index(vaddr)).GetPresent())
        return HandleResult::PageUnmapped;

    if unlikely(!pml3p->operator[](VmmArc::GetPml3Index(vaddr)).GetPresent())
        return HandleResult::PageUnmapped;

    Pml2Entry & pml2e = pml2p->operator[](VmmArc::GetPml2Index(vaddr));

    if unlikely(!pml2e.GetPresent())
        return HandleResult::PageUnmapped;

    if (pml2e.GetPageSize())
        return cbk(reinterpret_cast<PmlCommonEntry *>(&pml2e), 2);

    Pml1Entry & pml1e = pml1p->operator[](VmmArc::GetPml1Index(vaddr));

    if unlikely(!pml1e.GetPresent())
        return HandleResult::PageUnmapped;

    return cbk(reinterpret_cast<PmlCommonEntry *>(&pml1e), 1);
}

template<typename cbk_t>
static __hot inline Handle TryTranslate(Process * proc
    , uintptr_t const vaddr
    , cbk_t cbk
    , bool const lockHeap)
{
    if unlikely((vaddr >= VmmArc::FractalStart && vaddr < VmmArc::FractalEnd     )
             || (vaddr >= VmmArc::LowerHalfEnd && vaddr < VmmArc::HigherHalfStart))
        return HandleResult::PageMapIllegalRange;

    //  No alignment check will be performed here.

    if unlikely(proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    bool const nonLocal = (vaddr < VmmArc::LowerHalfEnd) && !Vmm::IsActive(proc);

    withInterrupts (false)  //  Interrupt-guarded.
        return TranslateInternal(proc, vaddr, cbk, lockHeap, true, nonLocal);

    __unreachable_code;
}

static __hot Handle MapPageInternal(Process * const proc
    , uintptr_t const vaddr, paddr_t paddr
    , FrameSize const size
    , MemoryFlags const flags
    , bool const lockHeap
    , bool const lockAlien
    , bool const nonLocal)
{
    uint16_t ind;   //  Used to hold the current index.

    Pml4 * pml4p; Pml3 * pml3p; Pml2 * pml2p; Pml1 * pml1p;

    Spinlock<> * alienLock = nullptr, * heapLock = nullptr;

    if (lockAlien && nonLocal && CpuDataSetUp)
        alienLock = &(Cpu::GetProcess()->AlienPagingTablesLock);

    LockGuardFlexible<Spinlock<> > pml4Lg {alienLock};

    if (nonLocal)
    {
        Alienate(proc);

        pml4p = VmmArc::GetAlienPml4();
        pml3p = VmmArc::GetAlienPml3(vaddr);
        pml2p = VmmArc::GetAlienPml2(vaddr);
        pml1p = VmmArc::GetAlienPml1(vaddr);

        if (!CpuDataSetUp || proc->PagingTable != Cpu::GetData()->LastAlienPml4)
        {
            CpuInstructions::InvalidateTlb(pml4p);
            CpuInstructions::InvalidateTlb(pml3p);
            CpuInstructions::InvalidateTlb(pml2p);
            CpuInstructions::InvalidateTlb(pml1p);

            //  Invalidate all!

            if (CpuDataSetUp)
                Cpu::GetData()->LastAlienPml4 = proc->PagingTable;
        }
    }
    else
    {
        pml4p = VmmArc::GetLocalPml4();
        pml3p = VmmArc::GetLocalPml3(vaddr);
        pml2p = VmmArc::GetLocalPml2(vaddr);
        pml1p = VmmArc::GetLocalPml1(vaddr);
    }

    if (lockHeap)
        heapLock = (vaddr < VmmArc::LowerHalfEnd
            ? &(proc->LocalTablesLock)
            : &(VmmArc::KernelHeapLock));

    LockGuardFlexible<Spinlock<> > heapLg {heapLock};

    ind = VmmArc::GetPml4Index(vaddr);

    if unlikely(!pml4p->operator[](ind).GetPresent())
    {
        //  So there's no PML4e. Means all PML1-3 need allocation.

        //  First grab a PML3.

        paddr_t const newPml3 = Pmm::AllocateFrame(1);

        if (newPml3 == nullpaddr)
            return HandleResult::OutOfMemory;

        pml4p->operator[](ind) = Pml4Entry(newPml3, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        //  Then a PML2.

        paddr_t const newPml2 = Pmm::AllocateFrame(1);

        if (newPml2 == nullpaddr)
        {
            Pmm::FreeFrame(newPml3);
            //  Yes, clean up.

            return HandleResult::OutOfMemory;
        }

        memset(pml3p, 0, PageSize);
        pml3p->operator[](VmmArc::GetPml3Index(vaddr)) = Pml3Entry(newPml2, true, true, true, false);
        //  First clean, then assign an entry.

        //  And finish by moving on.

        memset(pml2p, 0, PageSize);

        goto do_pml2e;
    }

    ind = VmmArc::GetPml3Index(vaddr);

    if unlikely(!pml3p->operator[](ind).GetPresent())
    {
        //  Just grab a PML2.

        paddr_t const newPml2 = Pmm::AllocateFrame(1);

        if (newPml2 == nullpaddr)
            return HandleResult::OutOfMemory;

        pml3p->operator[](ind) = Pml3Entry(newPml2, true, true, true, false);
        //  First clean, then assign an entry.

        memset(pml2p, 0, PageSize);
    }
    
do_pml2e:
    ind = VmmArc::GetPml2Index(vaddr);

    if unlikely(!pml2p->operator[](ind).GetPresent())
    {
        if likely(size == FrameSize::_4KiB)
        {
            paddr_t const newPml1 = Pmm::AllocateFrame(1);

            if (newPml1 == nullpaddr)
                return HandleResult::OutOfMemory;

            pml2p->operator[](ind) = Pml2Entry(newPml1, true, true, true, false);
            //  Present, writable, user-accessible, executable.

            memset(pml1p, 0, PageSize);

            goto do_pml1e;
        }
        else
        {
            pml2p->operator[](ind) = Pml2Entry(paddr, true
                , 0 != (flags & MemoryFlags::Writable)
                , 0 != (flags & MemoryFlags::Userland)
                , 0 != (flags & MemoryFlags::Global)
                , 0 == (flags & MemoryFlags::Executable) && VmmArc::NX);
            //  Present, writable, user-accessible, global, executable.

            return HandleResult::Okay;
        }
    }
    
    ind = VmmArc::GetPml1Index(vaddr);

    if unlikely(pml1p->operator[](ind).GetPresent())
        return HandleResult::PageMapped;

    if likely(size == FrameSize::_4KiB)
    {
    do_pml1e:
        pml1p->operator[](VmmArc::GetPml1Index(vaddr)) = Pml1Entry(paddr, true
            , 0 != (flags & MemoryFlags::Writable)
            , 0 != (flags & MemoryFlags::Userland)
            , 0 != (flags & MemoryFlags::Global)
            , 0 == (flags & MemoryFlags::Executable) && VmmArc::NX);
        //  Present, writable, user-accessible, global, executable.

        return HandleResult::Okay;
    }
    else
        return HandleResult::PageMapped;
}

Handle Vmm::MapPage(Process * proc, uintptr_t const vaddr, paddr_t paddr
    , FrameSize size, MemoryFlags const flags, MemoryMapOptions opts)
{
    if unlikely((vaddr >= VmmArc::FractalStart && vaddr < VmmArc::FractalEnd     )
             || (vaddr >= VmmArc::LowerHalfEnd && vaddr < VmmArc::HigherHalfStart))
        return HandleResult::PageMapIllegalRange;

    switch (size)
    {
    case FrameSize::_64KiB: //  TODO: Map 4-KiB pages to provide this.
    case FrameSize::_4MiB:  //  TODO: Map 2-MiB pages to provide this.
    case FrameSize::_1GiB:
        ASSERT(false, "A request was made for a frame size which is not supported by this architecture.");
        break;

    case FrameSize::_4KiB:
        if unlikely(!Is4KiBAligned(vaddr) || !Is4KiBAligned(paddr))
            return HandleResult::AlignmentFailure;

        break;

    case FrameSize::_2MiB:
        if unlikely(!Is2MiBAligned(vaddr) || !Is2MiBAligned(paddr))
            return HandleResult::AlignmentFailure;

        break;

    default:
        ASSERT(false, "Invalid value provided as frame size: %us", (size_t)size);
        break;
    }

    Handle res;

    if (proc == nullptr)
        proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    bool const nonLocal = (vaddr < VmmArc::LowerHalfEnd) && !Vmm::IsActive(proc);

    withInterrupts (false)
        res = MapPageInternal(proc, vaddr, paddr, size, flags
            , 0 == (opts & MemoryMapOptions::NoLocking)
            , true, nonLocal);

    if unlikely(res != HandleResult::Okay)
        return res;

    if (0 == (opts & MemoryMapOptions::NoReferenceCounting))
    {
        res = Pmm::AdjustReferenceCount(paddr, 1);
        //  The page still may not be in an allocation space, but that is taken
        //  care of by the PMM.

        if (res.IsResult(HandleResult::PagesOutOfAllocatorRange))
            return HandleResult::Okay;
        else
            return res;
    }
    else
        return HandleResult::Okay;
}

Handle Vmm::MapRange(Process * proc
    , uintptr_t vaddr, paddr_t paddr, size_t size
    , MemoryFlags const flags
    , MemoryMapOptions opts)
{
    if unlikely((vaddr + size > VmmArc::FractalStart && vaddr < VmmArc::FractalEnd     )
             || (vaddr + size > VmmArc::LowerHalfEnd && vaddr < VmmArc::HigherHalfStart))
        return HandleResult::PageMapIllegalRange;

    if unlikely(!Is4KiBAligned(vaddr) || !Is4KiBAligned(paddr) || !Is4KiBAligned(size))
        return HandleResult::AlignmentFailure;

    ASSERT(0 != (opts & MemoryMapOptions::NoReferenceCounting)
        , "Reference counting must be explicitly disabled when mapping a memory range!");

    vaddr_t const end = vaddr + size;

    Handle res;

    if (proc == nullptr)
        proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    bool const nonLocal = (vaddr < VmmArc::LowerHalfEnd) && !Vmm::IsActive(proc);

    Spinlock<> * alienLock = nullptr, * heapLock = nullptr;

    if (nonLocal && CpuDataSetUp)
        alienLock = &(Cpu::GetProcess()->AlienPagingTablesLock);

    if (0 == (opts & MemoryMapOptions::NoLocking))
        heapLock = (vaddr < VmmArc::LowerHalfEnd
            ? &(proc->LocalTablesLock)
            : &(VmmArc::KernelHeapLock));

    withInterrupts (false)
    {
        //  THE SCOPE IS OPTIONALLY LOCK-GUARDED AS WELL!

        LockGuardFlexible<Spinlock<> > pml4Lg {alienLock};
        LockGuardFlexible<Spinlock<> > heapLg {heapLock};

        if ((vaddr & (LargePageSize - 1)) == (paddr & (LargePageSize - 1)))
        {
            //  Wow, so the alignment matches. This means 2-MiB mappings can be used!
            //  First, map the small pages until a 2-MiB aligned address is reached.

            for (/* nothing */; vaddr < end && !Is2MiBAligned(vaddr); vaddr += PageSize, paddr += PageSize)
            {
                res = MapPageInternal(proc, vaddr, paddr
                    , FrameSize::_4KiB, flags
                    , false, false, nonLocal);

                if unlikely(res != HandleResult::Okay)
                    return res;
            }

            //  Now map as many 2-MiB pages as possible.

            vaddr_t const endRD = RoundDown(end, LargePageSize);
            //  endRD = end rounded down to 2-MiB

            for (/* nothing */; vaddr < endRD; vaddr += LargePageSize, paddr += LargePageSize)
            {
                res = MapPageInternal(proc, vaddr, paddr
                    , FrameSize::_2MiB, flags
                    , false, false, nonLocal);

                if unlikely(res != HandleResult::Okay)
                    return res;
            }
        }

        //  If alignment does not match, or if there's anything left after mapping
        //  as many 2-MiB pages as possible, this last loop will take care of it.

        for (/* nothing */; vaddr < end; vaddr += PageSize, paddr += PageSize)
        {
            res = MapPageInternal(proc, vaddr, paddr
                , FrameSize::_4KiB, flags
                , false, false, nonLocal);

            if unlikely(res != HandleResult::Okay)
                return res;
        }
    }

    return HandleResult::Okay;
}

Handle Vmm::UnmapPage(Process * proc, uintptr_t const vaddr
    , paddr_t & paddr, FrameSize & size, MemoryMapOptions opts)
{
    if (proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    Handle res = TryTranslate(proc, vaddr, [&paddr, &size](PmlCommonEntry * pE, int level)
    {
        paddr = pE->GetAddress();
        size = level == 1 ? FrameSize::_4KiB : FrameSize::_2MiB;

        *pE = PmlCommonEntry();
        //  Null.

        return HandleResult::Okay;
    }, 0 == (opts & MemoryMapOptions::NoLocking));

    if unlikely(!res.IsOkayResult())
        return res;

    //  The rest is done outside of the lambda because locks are unnecessary.

    Vmm::InvalidatePage(proc, vaddr, true);

    if (0 == (opts & MemoryMapOptions::NoReferenceCounting))
        Pmm::AdjustReferenceCount(paddr, -1);

    return HandleResult::Okay;
}

Handle Vmm::UnmapRange(Process * proc
    , uintptr_t vaddr, size_t size
    , MemoryMapOptions opts)
{
    if unlikely((vaddr + size > VmmArc::FractalStart && vaddr < VmmArc::FractalEnd     )
             || (vaddr + size > VmmArc::LowerHalfEnd && vaddr < VmmArc::HigherHalfStart))
        return HandleResult::PageMapIllegalRange;

    if unlikely(!Is4KiBAligned(vaddr) || !Is4KiBAligned(size))
        return HandleResult::AlignmentFailure;

    ASSERT(0 != (opts & MemoryMapOptions::NoReferenceCounting)
        , "Reference counting must be explicitly disabled when unmapping a memory range!");
    //  TODO: Implement this recursively or something, so this can be done.
    //  It's vital.

    ASSERT(0 == (opts & MemoryMapOptions::PreciseUnmapping)
        , "Precise unmapping is not supported yet.");

    vaddr_t const end = vaddr + size;

    if (proc == nullptr)
        proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    bool const nonLocal = (vaddr < VmmArc::LowerHalfEnd) && !Vmm::IsActive(proc);

    Spinlock<> * alienLock = nullptr, * heapLock = nullptr;

    if (nonLocal && CpuDataSetUp)
        alienLock = &(Cpu::GetProcess()->AlienPagingTablesLock);

    if (0 == (opts & MemoryMapOptions::NoLocking))
        heapLock = (vaddr < VmmArc::LowerHalfEnd
            ? &(proc->LocalTablesLock)
            : &(VmmArc::KernelHeapLock));

    withInterrupts (false)
    {
        //  THE SCOPE IS OPTIONALLY LOCK-GUARDED AS WELL!

        LockGuardFlexible<Spinlock<> > pml4Lg {alienLock};
        LockGuardFlexible<Spinlock<> > heapLg {heapLock};

        while (vaddr < end)
        {
            paddr_t paddr;
            FrameSize fSize;

            Handle res = TranslateInternal(proc, vaddr, [&paddr, &fSize](PmlCommonEntry * pE, int level)
            {
                paddr = pE->GetAddress();
                fSize = level == 1 ? FrameSize::_4KiB : FrameSize::_2MiB;

                *pE = PmlCommonEntry();
                //  Null.

                return HandleResult::Okay;
            }, false, false, nonLocal);

            if unlikely(res != HandleResult::Okay)
                return res;

            if (fSize == FrameSize::_4KiB)
                vaddr += PageSize;
            else
                vaddr = RoundUp(vaddr + 1, LargePageSize);
        }
    }

    return HandleResult::Okay;
}

Handle Vmm::InvalidatePage(Process * proc, uintptr_t const vaddr
    , bool const broadcast)
{
    if unlikely(proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    CpuInstructions::InvalidateTlb(reinterpret_cast<void const *>(vaddr));

    if (broadcast)
    {
        //  TODO: Broadcast!

        //  If PCID is enabled, all invalidations should be broadcast so INVPID
        //  or whatever can be used. Without PCID, the broadcast should only be
        //  performed with kernel data (later changeable if the kernel proves
        //  stable enough) or when there's more than one core where the current
        //  process is active on.
    }

    return HandleResult::Okay;
}

Handle Vmm::Translate(Execution::Process * proc, uintptr_t const vaddr, paddr_t & paddr, bool const lock)
{
    return TryTranslate(proc, vaddr, [&paddr](PmlCommonEntry * pE, int level)
    {
        paddr = pE->GetAddress();

        return HandleResult::Okay;
    }, lock);
}

Handle Vmm::HandlePageFault(Execution::Process * proc
    , uintptr_t const vaddr, PageFaultFlags const flags)
{
    if unlikely(0 != (flags & PageFaultFlags::Present))
        return HandleResult::Failed;
    //  Page is present. This means this is an access (write/execute) failure.

    if (proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    Memory::Vas * vas = &(proc->Vas);

    Handle res = HandleResult::Okay;

    paddr_t paddr;

    vaddr_t const vaddr_algn = RoundDown(vaddr, PageSize);
    MemoryRegion * reg;

    vas->Lock.AcquireAsReader();

#define RETURN(HRES) do { res = HandleResult::HRES; goto end; } while (false)

    if (vas->LastSearched != nullptr && vas->LastSearched->Contains(vaddr))
        reg = vas->LastSearched;
        //  No need to check whether anything can be allocated in the page or not.
    else
    {
        reg = vas->FindRegion(vaddr);

        if unlikely( reg == nullptr
                 || (reg->Type & MemoryAllocationOptions::PurposeMask) == MemoryAllocationOptions::Free)
            RETURN(ArgumentOutOfRange);
        //  Either of these conditions means this page fault was caused by a hit on
        //  unallocated/freed memory.

        if unlikely((reg->Type & MemoryAllocationOptions::StrategyMask) != MemoryAllocationOptions::AllocateOnDemand)
            RETURN(PageUndemandable);
        //  Regions which aren't allocated on demand aren't covered by this handler.
    }

    if unlikely((0 != (reg->Type & MemoryAllocationOptions::GuardLow ) && vaddr_algn <  (reg->Range.Start + PageSize))
             || (0 != (reg->Type & MemoryAllocationOptions::GuardHigh) && vaddr_algn >= (reg->Range.End   - PageSize)))
        RETURN(PageGuard);
    //  Thie hit seems to have landed on a guard page.

    if unlikely((0 != (flags & PageFaultFlags::Execute ) && 0 == (reg->Flags & MemoryFlags::Executable))
             || (0 != (flags & PageFaultFlags::Userland) && 0 == (reg->Flags & MemoryFlags::Userland  )))
        RETURN(Failed);
    //  So this was either an attempt to execute a non-executable page, or to
    //  access (in any way) a supervisor page from userland.

    //  Reaching this point means this page is meant to be allocated.

    vas->LastSearched = reg;
    //  Make the next operation potentially faster. This is done even if the
    //  following allocation fails, because it doesn't affect the correctness of
    //  the VAS.

    paddr = Pmm::AllocateFrame();

    if unlikely(paddr == nullpaddr)
        RETURN(OutOfMemory);
    //  Okay... Out of memory... Bad.
    //  TODO: Handle this.

    res = Vmm::MapPage(proc, vaddr_algn, paddr, reg->Flags);

    if unlikely(!res.IsOkayResult())
    {
        //  Okay, this really shouldn't fail.

        Pmm::FreeFrame(paddr);
        //  Get rid of the physical page if mapping failed. :frown:
    }

    vas->Lock.ReleaseAsReader();

    if likely(vaddr < KernelStart)
    {
        //  This was a request in userland, therefore the page contents need to
        //  be TERMINATED.

        withWriteProtect (false)
            memset(reinterpret_cast<void *>(vaddr_algn), 0xCA, PageSize);
        //  It's all CACA!
    }

    return res;

#undef RETURN
end:
    vas->Lock.ReleaseAsReader();

    return res;
}

/*  Allocation  */

Handle Vmm::AllocatePages(Process * proc, size_t const count
    , MemoryAllocationOptions const type
    , MemoryFlags const flags
    , MemoryContent content
    , uintptr_t & vaddr)
{
    if (proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    if (MemoryAllocationOptions::AllocateOnDemand == (type & MemoryAllocationOptions::AllocateOnDemand))
    {
        if (0 != (type & MemoryAllocationOptions::VirtualUser))
            return proc->Vas.Allocate(vaddr, count, flags, content, type);
        else
            return HandleResult::NotImplemented;
    }
    else if (0 != (type & MemoryAllocationOptions::Commit))
    {
        Handle res; //  Intermediary result.

        size_t const  lowerOffset = (0 != (type & MemoryAllocationOptions::GuardLow))
            ? PageSize : 0;
        size_t const higherOffset = (0 != (type & MemoryAllocationOptions::GuardHigh))
            ? PageSize : 0;

        Spinlock<> * heapLock;

        vaddr_t ret;
        size_t const size = count * PageSize;

        if (0 != (type & MemoryAllocationOptions::VirtualUser))
        {
            res = proc->Vas.Allocate(vaddr, count, flags, content, type);

            if unlikely(!res.IsOkayResult())
                return res;

            ret = vaddr - lowerOffset;
            heapLock = &(proc->LocalTablesLock);
        }
        else
        {
            if unlikely(KernelHeapOverflown)
                return HandleResult::UnsupportedOperation;
            //  Not supported yet.

            if (vaddr == nullvaddr)
                ret = KernelHeapCursor.FetchAdd(size + lowerOffset + higherOffset);
            else
            {
                ret = vaddr;
                vaddr_t newCursor = vaddr + size + lowerOffset + higherOffset;

                if likely(!KernelHeapCursor.CmpXchgStrong(ret, newCursor))
                    return HandleResult::Failed;
            }

            if unlikely(ret > VmmArc::KernelHeapEnd)
            {
                KernelHeapCursor.CmpXchgStrong(ret, ret - VmmArc::KernelHeapLength);
                KernelHeapOverflown = true;

                return HandleResult::UnsupportedOperation;
                //  Not supported yet.
            }

            vaddr = ret + lowerOffset;
            heapLock = &(VmmArc::KernelHeapLock);
        }

        InterruptGuard<> intGuard;
        //  Guard the rest of the scope from interrupts.

        bool allocSucceeded = true;

        LockGuard<Spinlock<> > heapLg {*heapLock};
        //  Note: this ain't flexible because heapLock ain't gonna be null.

        size_t offset;
        for (offset = 0; offset < size; offset += PageSize)
        {
            paddr_t const paddr = Pmm::AllocateFrame();

            if unlikely(paddr == nullpaddr) { allocSucceeded = false; break; }

            res = Vmm::MapPage(proc, ret + lowerOffset + offset, paddr
                , flags, MemoryMapOptions::NoLocking);

            if unlikely(!res.IsOkayResult()) { allocSucceeded = false; break; }
        }

        if likely(allocSucceeded)
        {
            if likely(0 != (type & MemoryAllocationOptions::VirtualUser))
                withWriteProtect (false)
                    memset(reinterpret_cast<void *>(vaddr), 0xCA, size);

            return HandleResult::Okay;
        }
        else
        {
            //  So, the allocation failed. Now all the pages that were allocated
            //  need to be unmapped.

            do
            {
                res = Vmm::UnmapPage(proc, ret + lowerOffset + offset, MemoryMapOptions::NoLocking);

                if unlikely(!res.IsOkayResult() && !res.IsResult(HandleResult::PageUnmapped))
                    return res;

                offset -= PageSize;
            } while (offset > 0);

            return HandleResult::OutOfMemory;
        }
    }
    else // Means it's used or it'll just be reserved.
    {
        if (0 != (type & MemoryAllocationOptions::VirtualUser))
            return proc->Vas.Allocate(vaddr, count, flags, content, type);
        //  Easy-peasy.

        size_t const  lowerOffset = (0 != (type & MemoryAllocationOptions::GuardLow))
            ? PageSize : 0;
        size_t const higherOffset = (0 != (type & MemoryAllocationOptions::GuardHigh))
            ? PageSize : 0;

        if unlikely(KernelHeapOverflown)
        {
            return HandleResult::UnsupportedOperation;
            //  Not supported yet.
        }

        vaddr_t ret = KernelHeapCursor.FetchAdd(count * PageSize + lowerOffset + higherOffset);

        if unlikely(ret > VmmArc::KernelHeapEnd)
        {
            KernelHeapCursor.CmpXchgStrong(ret, ret - VmmArc::KernelHeapLength);
            KernelHeapOverflown = true;

            vaddr = nullvaddr;
            return HandleResult::UnsupportedOperation;
            //  Not supported yet.
        }

        vaddr = ret + lowerOffset;

        return HandleResult::Okay;
    }
}

Handle Vmm::FreePages(Process * proc, uintptr_t const vaddr, size_t const count)
{
    return HandleResult::NotImplemented;
}

/*  Flags  */

Handle Vmm::CheckMemoryRegion(Execution::Process * proc
    , uintptr_t addr, size_t size, MemoryCheckType type)
{
    if (proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    Memory::Vas * vas = &(proc->Vas);

    if (addr >= UserlandStart && addr < UserlandEnd)
        vas = &(proc->Vas);
    else if (addr >= KernelStart && addr < KernelEnd)
        return HandleResult::NotImplemented;    //  TODO: Kernel VAS
    else
        return HandleResult::ArgumentOutOfRange;

    vaddr_t const vaddr_end = addr + size;

    Handle res = HandleResult::Okay;

    PointerAndSize const chkrng = {addr, size};
    MemoryFlags const mandatoryFlags = 
        (  0 != (type & MemoryCheckType::Writable) ? MemoryFlags::Writable : MemoryFlags::None)
        | (0 != (type & MemoryCheckType::Userland) ? MemoryFlags::Userland : MemoryFlags::None);

    MemoryRegion * reg;

#define RETURN(HRES) do { res = HandleResult::HRES; goto end; } while (false)

    vas->Lock.AcquireAsReader();

    if (vas->LastSearched != nullptr && vas->LastSearched->Contains(addr))
        reg = vas->LastSearched;
        //  No need to check whether anything can be allocated in the page or not.
    else
    {
    find_region:
        reg = vas->FindRegion(addr);

        if unlikely(reg == nullptr)
            RETURN(ArgumentOutOfRange);

        if unlikely((0 != (type & MemoryCheckType::Free))
                 && (MemoryAllocationOptions::Free == (reg->Type & MemoryAllocationOptions::PurposeMask)))
            goto next_region;
        //  So free memory was asked for, and this is a free region. Let's move on.

        if unlikely((reg->Type & MemoryAllocationOptions::StrategyMask) == MemoryAllocationOptions::Reserve)
            RETURN(PageReserved);
        //  Regions which are reserved cannot be accessed like this.
    }

    if unlikely((0 != (reg->Type & MemoryAllocationOptions::GuardLow ) && DoRangesIntersect(chkrng, {reg->Range.Start         , PageSize}))
             || (0 != (reg->Type & MemoryAllocationOptions::GuardHigh) && DoRangesIntersect(chkrng, {reg->Range.End - PageSize, PageSize})))
        RETURN(PageGuard);
    //  Thie region overlaps a guard page.

    if unlikely((reg->Flags & mandatoryFlags) != mandatoryFlags)
        RETURN(Failed);

    if unlikely((0 != (type & MemoryCheckType::Private))
         && (MemoryAllocationOptions::Share   == (reg->Type & MemoryAllocationOptions::PurposeMask)
          || MemoryAllocationOptions::Runtime == (reg->Type & MemoryAllocationOptions::PurposeMask)))
        RETURN(Failed);
    //  Private memory was asked for, and this is shared or part of the runtime.

    //  Reaching this point means this region is passing the check.

next_region:
    if unlikely(vaddr_end > reg->Range.End)
    {
        size_t const diff = reg->Range.End - addr;

        addr += diff;   //  Equivalent to addr = reg->Range.End
        size -= diff;

        goto find_region;
    }

#undef RETURN
end:
    vas->Lock.ReleaseAsReader();

    return res;
}

Handle Vmm::GetPageFlags(Process * proc, uintptr_t const vaddr
    , MemoryFlags & flags, bool const lock)
{
    return TryTranslate(proc, vaddr, [&flags](PmlCommonEntry * pE, int level)
    {
        PmlCommonEntry const e = *pE;
        MemoryFlags f = MemoryFlags::None;

        if (  e.GetGlobal())            f |= MemoryFlags::Global;
        if (  e.GetUserland())          f |= MemoryFlags::Userland;
        if (  e.GetWritable())          f |= MemoryFlags::Writable;
        if (!(e.GetXd() && VmmArc::NX)) f |= MemoryFlags::Executable;

        flags = f;

        return HandleResult::Okay;
    }, lock);
}

Handle Vmm::SetPageFlags(Process * proc, uintptr_t const vaddr
    , MemoryFlags const flags, bool const lock)
{
    if (proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    Handle res = TryTranslate(proc, vaddr, [flags](PmlCommonEntry * pE, int level)
    {
        PmlCommonEntry e = *pE;

        e.SetGlobal( ((MemoryFlags::Global     & flags) != 0))
        .SetUserland(((MemoryFlags::Userland   & flags) != 0))
        .SetWritable(((MemoryFlags::Writable   & flags) != 0))
        .SetXd( VmmArc::NX & ((MemoryFlags::Executable & flags) == 0));

        *pE = e;

        return HandleResult::Okay;
    }, lock);

    if (!res.IsOkayResult())
        return res;

    return Vmm::InvalidatePage(proc, vaddr, true);
}
