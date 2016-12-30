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

#include "memory/vmm.hpp"
#include "memory/vmm.arc.hpp"
#include "memory/pmm.hpp"
#include "memory/pmm.arc.hpp"
#include <memory/object_allocator_pools_heap.hpp>
#include "kernel.hpp"
#include "cores.hpp"
#include "mailbox.hpp"

#include <beel/interrupt.state.hpp>
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

bool VmmArc::Page1GB = false;
bool VmmArc::NX = false;
bool VmmArc::PCID = false;
__thread paddr_t VmmArc::LastAlienPml4;

static uintptr_t BootstrapKVasAddr;
static size_t const BootstrapKVasPageCount = 3;

inline void Alienate(Process * proc)
{
    Pml4 & pml4 = *(VmmArc::GetLocalPml4());

    pml4[VmmArc::AlienFractalIndex] = Pml4Entry(proc->PagingTable, true, true, false, VmmArc::NX);
}

/*  Statics  */

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
    vaddr_t curLoc = VmmArc::KernelHeapStart; //  Used for serial allocation.
    Handle res; //  Temporary result.

    do
    {
        if ((vaddr_t)cur < VmmArc::HigherHalfStart && pendingLinksMapping)
        {
            res = Vmm::MapPage(bootstrapProc, curLoc, RoundDown((paddr_t)cur, PageSize)
                , MemoryFlags::Global | MemoryFlags::Writable
                , MemoryMapOptions::NoLocking | MemoryMapOptions::NoReferenceCounting);
            //  Global because it's shared by processes, and writable for hotplug.

            ASSERTX(res.IsOkayResult()
                , "Failed to map links between allocation spaces.")
                (res)XEND;
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

        res = Vmm::MapRange(bootstrapProc
            , curLoc, pasStart, RoundUp(controlStructuresSize, PageSize)
            , MemoryFlags::Global | MemoryFlags::Writable
            , MemoryMapOptions::NoLocking | MemoryMapOptions::NoReferenceCounting);

        ASSERTX(res.IsOkayResult()
            , "Failed to map range %Xp to %XP (%Xs bytes): %H"
            , curLoc
            , pasStart
            , controlStructuresSize
            , res)XEND;
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

            ASSERTX(res.IsOkayResult()
                , "Failed to map split frame subdescriptor page #%u8 (%Xp to %XP): %H"
                , i
                , curLoc
                , lDesc->SubDescriptors
                , res)XEND;

            withLock (cur->SplitLocker)
                lDesc->SubDescriptors = reinterpret_cast<SmallFrameDescriptor *>(curLoc);
        }

    } while ((cur = cur->Next) != nullptr);

    Pml4 & pml4 = *(VmmArc::GetLocalPml4());

    for (uint16_t i = 0; i < 256; ++i)
        pml4[i] = Pml4Entry();
    //  Getting rid of those naughty identity maps.

    Vmm::Switch(nullptr, bootstrapProc);
    //  Re-activate, to flush the identity maps.

    BootstrapKVasAddr = curLoc;

    for (size_t i = 0; i < BootstrapKVasPageCount; ++i)
    {
        paddr_t const paddr = Pmm::AllocateFrame();

        ASSERT_NEQ(nullpaddr, paddr);

        res = Vmm::MapPage(bootstrapProc
            , BootstrapKVasAddr + i * PageSize, paddr
            , MemoryFlags::Global | MemoryFlags::Writable
            , MemoryMapOptions::NoLocking);

        ASSERTX(res.IsOkayResult(), "Failed to map page for bootstrap KVAS descriptor pool.")
            (res)("page number", i + 1)XEND;
    }

    curLoc += BootstrapKVasPageCount * PageSize;

    // MSG("Instancing KVAS.%n");

    new (&KVas) KernelVas();
    //  Just making sure.

    // MSG("Initializing KVAS.%n");

    res = KVas.Initialize(VmmArc::KernelHeapStart, VmmArc::KernelHeapEnd
        , &AcquirePoolForVas, &EnlargePoolForVas, &ReleasePoolFromKernelHeap
        , PoolReleaseOptions::KeepOne);
    //  Prepare the VAS for usage.

    ASSERTX(res.IsOkayResult(), "Failed to initialize the kernel VAS.")(res)XEND;

    // MSG("Allocating VMM bootstrap region in KVAS.%n");

    vaddr_t khs = VmmArc::KernelHeapStart;

    res = KVas.Allocate(khs
        , curLoc - khs
        , MemoryFlags::Global | MemoryFlags:: Writable
        , MemoryContent::VmmBootstrap
        , MemoryAllocationOptions::Permanent);

    ASSERTX(res.IsOkayResult(), "Failed to allocate VMM bootstrap area descriptor in kernel VAS.")(res)XEND;

    KVas.Bootstrapping = false;
    //  Should be ready!

    // MSG("Finished VMM init!%n");

    return res;
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
            VmmArc::LastAlienPml4 = pml4_paddr;
    }

    return proc->Vas.Initialize(UserlandStart, UserlandEnd
        , &AcquirePoolInKernelHeap, &EnlargePoolInKernelHeap, &ReleasePoolFromKernelHeap);
}

/*  Activation and Status  */

Handle Vmm::Switch(Process * const oldProc, Process * const newProc)
{
    (void)oldProc;

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
static __hot __noinline Handle TranslateInternal(Process * proc
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

        if (!CpuDataSetUp || proc->PagingTable != VmmArc::LastAlienPml4)
        {
            CpuInstructions::InvalidateTlb(pml4p);
            CpuInstructions::InvalidateTlb(pml3p);
            CpuInstructions::InvalidateTlb(pml2p);
            CpuInstructions::InvalidateTlb(pml1p);

            //  Invalidate all!

            if (CpuDataSetUp)
                VmmArc::LastAlienPml4 = proc->PagingTable;
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
        heapLock = vaddr < VmmArc::LowerHalfEnd ? &proc->LocalTablesLock : &Vmm::KernelHeapLock;

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

        if (!CpuDataSetUp || proc->PagingTable != VmmArc::LastAlienPml4)
        {
            CpuInstructions::InvalidateTlb(pml4p);
            CpuInstructions::InvalidateTlb(pml3p);
            CpuInstructions::InvalidateTlb(pml2p);
            CpuInstructions::InvalidateTlb(pml1p);

            //  Invalidate all!

            if (CpuDataSetUp)
                VmmArc::LastAlienPml4 = proc->PagingTable;
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
            : &(Vmm::KernelHeapLock));

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
        FAIL("A request was made for a frame size which is not supported by this architecture.");
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
        FAIL("Invalid value provided as frame size: %us", (size_t)size);
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

    ASSERTX(0 != (opts & MemoryMapOptions::NoReferenceCounting)
        , "Reference counting must be explicitly disabled when mapping a memory range!")XEND;

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
            : &(Vmm::KernelHeapLock));

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

    if unlikely(res != HandleResult::Okay)
        return res;

    //  The rest is done outside of the lambda because locks are unnecessary.

    Vmm::InvalidatePage(proc, vaddr, true);

    if (0 == (opts & MemoryMapOptions::NoReferenceCounting))
        Pmm::AdjustReferenceCount(paddr, -1);

    return HandleResult::Okay;
}

struct RecursiveUnmapState
{
    Execution::Process * const Process;
    vaddr_t Address;
    vaddr_t const EndAddress;
    Spinlock<> * const AlienLock;
    Spinlock<> * const HeapLock;
    Beelzebub::InterruptState const InterruptState;
    bool const NonLocal, Invalidate, Broadcast, CountReferences;
};

struct HybridPageNode : public Vmm::PageNode
{
    inline HybridPageNode(vaddr_t vaddr, paddr_t paddr) : Vmm::PageNode( vaddr), Frame(paddr) { }
    inline HybridPageNode(vaddr_t vaddr, paddr_t paddr, HybridPageNode const * next) : Vmm::PageNode(vaddr, next), Frame(paddr) { }

    paddr_t const Frame;
    
    inline HybridPageNode const * GetNext() const
    {
        return static_cast<HybridPageNode const *>(this->Next);
    }
};

static __hot void FreeAfterInvalidation(Vmm::InvalidationInfo const * const inf
    , bool caller)
{
    if (!caller)
        return;

    auto tmp = static_cast<HybridPageNode const *>(inf->Node);

    do
    {
        assert(tmp->Frame != nullpaddr);

        Handle res = Pmm::AdjustReferenceCount(tmp->Frame, -1);

        assert(res == HandleResult::Okay
            || res == HandleResult::PageReserved
            || res == HandleResult::PagesOutOfAllocatorRange)
            (res);
    } while ((tmp = tmp->GetNext()) != nullptr);
}

static __hot Handle UnmapRecursively(RecursiveUnmapState * const state
    , HybridPageNode const * node)
{
    vaddr_t next;
    paddr_t paddr = nullpaddr;
    FrameSize fSize = FrameSize::_1GiB;

    Handle res = TranslateInternal(state->Process, state->Address
        , [&paddr, &fSize](PmlCommonEntry * pE, int level)
        {
            paddr = pE->GetAddress();
            fSize = level == 1 ? FrameSize::_4KiB : FrameSize::_2MiB;

            *pE = PmlCommonEntry();
            //  Null.

            return HandleResult::Okay;
        }, false, false, state->NonLocal);

    HybridPageNode const newNode {state->Address, paddr, node};

    if unlikely(res != HandleResult::Okay)
        goto bail;

    node = &newNode;
    //  Yep, re-using a variable, evilishly.

    if (fSize == FrameSize::_4KiB)
        next = state->Address + PageSize;
    else
        next = RoundUp(state->Address + 1, LargePageSize);

    if (next >= state->EndAddress)
        goto bail;
    //  This means enough has been mapped!

    state->Address = next;

    res = UnmapRecursively(state, node);
    //  Note: `node` = &newNode;

    goto end;

bail:
    if (state->AlienLock != nullptr)
        state->AlienLock->Release();
    if (state->HeapLock != nullptr)
        state->HeapLock->Release();

    state->InterruptState.Restore();

    if (state->Invalidate)
        Vmm::InvalidateChain(state->Process, node, state->Broadcast
            , state->CountReferences ? FreeAfterInvalidation : nullptr);
    //  Easy..?

end:
    return res;
}

Handle Vmm::UnmapRange(Process * proc
    , uintptr_t vaddr, size_t size
    , MemoryMapOptions opts)
{
    if unlikely((vaddr + size > VmmArc::FractalStart && vaddr < VmmArc::FractalEnd     )
             || (vaddr + size > VmmArc::LowerHalfEnd && vaddr < VmmArc::HigherHalfStart))
        return HandleResult::PageMapIllegalRange;

    if unlikely(size == 0)
        return HandleResult::ArgumentOutOfRange;

    if unlikely(!Is4KiBAligned(vaddr) || !Is4KiBAligned(size))
        return HandleResult::AlignmentFailure;

    ASSERTX(0 == (opts & MemoryMapOptions::PreciseUnmapping)
        , "Precise unmapping is not supported yet.")XEND;

    if (proc == nullptr)
        proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    vaddr_t const endAddr = vaddr + size;
    bool const nonLocal = (vaddr < VmmArc::LowerHalfEnd) && !Vmm::IsActive(proc);
    bool const invalidate = 0 == (opts & MemoryMapOptions::NoInvalidation);
    bool const broadcast = 0 == (opts & MemoryMapOptions::NoBroadcasting);

    Spinlock<> * alienLock = nullptr, * heapLock = nullptr;

    if (nonLocal && CpuDataSetUp)
        alienLock = &(Cpu::GetProcess()->AlienPagingTablesLock);

    if (0 == (opts & MemoryMapOptions::NoLocking))
        heapLock = (vaddr < VmmArc::LowerHalfEnd
            ? &(proc->LocalTablesLock)
            : &(Vmm::KernelHeapLock));

    InterruptState int_cookie = InterruptState::Disable();

    if (alienLock != nullptr)
        alienLock->Acquire();
    if (heapLock != nullptr)
        heapLock->Acquire();

    RecursiveUnmapState state = {
        proc, vaddr, endAddr, alienLock, heapLock, int_cookie
        , nonLocal, invalidate, broadcast
        , 0 == (opts & MemoryMapOptions::NoReferenceCounting)
    };

    return UnmapRecursively(&state, nullptr);
}

template<bool caller>
static __hot void Invalidator(void * cookie)
{
    Vmm::InvalidationInfo const * const inf = (Vmm::InvalidationInfo const *)cookie;

    Vmm::PageNode const * tmp = inf->Node;

    do
    {
        CpuInstructions::InvalidateTlb(reinterpret_cast<void const *>(tmp->Address));
    } while ((tmp = tmp->Next) != nullptr);

    if (inf->After != nullptr)
        inf->After(inf, caller);
}

Handle Vmm::InvalidateChain(Process * proc, PageNode const * node, bool broadcast
    , Vmm::AfterInvalidationFunc after, void * cookie)
{
    if unlikely(proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    if (broadcast && ((node->Address >= UserlandStart && node->Address < UserlandEnd && proc->ActiveCoreCount == 1 && !VmmArc::PCID) || unlikely(!Mailbox::IsReady())))
        broadcast = false;

    InvalidationInfo info { proc, node, after, cookie };

    if (broadcast)
    {
        ALLOCATE_MAIL_BROADCAST(mail, &Invalidator<false>, &info);
        mail.SetAwait(true).Post(&Invalidator<true>, &info);

        //  Quite simple.
    }
    else
        Invalidator<true>(&info);

    return HandleResult::Okay;
}

Handle Vmm::Translate(Execution::Process * proc, uintptr_t const vaddr, paddr_t & paddr, bool const lock)
{
    return TryTranslate(proc, vaddr, [&paddr](PmlCommonEntry * pE, int level)
    {
        (void)level;

        paddr = pE->GetAddress();

        return HandleResult::Okay;
    }, lock);
}

/*  Flags  */

Handle Vmm::GetPageFlags(Process * proc, uintptr_t const vaddr
    , MemoryFlags & flags, bool const lock)
{
    return TryTranslate(proc, vaddr, [&flags](PmlCommonEntry * pE, int level)
    {
        (void)level;

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
        (void)level;
        
        PmlCommonEntry e = *pE;

        e.SetGlobal( ((MemoryFlags::Global     & flags) != 0))
        .SetUserland(((MemoryFlags::Userland   & flags) != 0))
        .SetWritable(((MemoryFlags::Writable   & flags) != 0))
        .SetXd( VmmArc::NX & ((MemoryFlags::Executable & flags) == 0));

        *pE = e;

        return HandleResult::Okay;
    }, lock);

    if (res != HandleResult::Okay)
        return res;

    return Vmm::InvalidatePage(proc, vaddr, true);
}

/*  Utils  */

Handle Vmm::AcquirePoolForVas(size_t objectSize, size_t headerSize
                            , size_t minimumObjects, ObjectPoolBase * & result)
{
    uintptr_t addr;
    size_t size;

    if likely(!KVas.Bootstrapping)
    {
        assert(headerSize >= sizeof(ObjectPoolBase)
            , "The given header size apprats to be lower than the size of an "
              "actual pool struct..?")
            (headerSize)(sizeof(ObjectPoolBase));

        addr = 0;
        size = RoundUp(objectSize * minimumObjects + headerSize, PageSize);

        Handle res = Vmm::AllocatePages(nullptr
            , size
            , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
            , MemoryFlags::Global | MemoryFlags::Writable
            , MemoryContent::VasDescriptors
            , addr);

        if (res != HandleResult::Okay)
            return res;
    }
    else
    {
        addr = BootstrapKVasAddr;
        size = BootstrapKVasPageCount * PageSize;
    }

    ObjectPoolBase volatile * volatile pool = (ObjectPoolBase *)(uintptr_t)addr;
    //  I use a local variable here so `result` isn't dereferenced every time.

    new (const_cast<ObjectPoolBase *>(pool)) ObjectPoolBase();
    //  Construct in place to initialize the fields.

    size_t const objectCount = (size - headerSize) / objectSize;
    //  TODO: Get rid of this division and make the loop below stop when the
    //  cursor reaches the end of the page(s).

    FillPool(pool, objectSize, headerSize, (obj_ind_t)objectCount);

    //  The pool was constructed in place, so the rest of the fields should
    //  be in a good state.

    result = const_cast<ObjectPoolBase *>(pool);

    return HandleResult::Okay;
}

Handle Vmm::EnlargePoolForVas(size_t objectSize, size_t headerSize
                            , size_t minimumExtraObjects, ObjectPoolBase * pool)
{
    size_t const oldSize = RoundUp(objectSize * pool->Capacity + headerSize, PageSize);
    size_t const newSize = RoundUp(objectSize * (pool->Capacity + minimumExtraObjects) + headerSize, PageSize);

    ASSERTX(newSize > oldSize
        , "New size should be larger than the old size of a pool that needs enlarging!%n"
          "It appears that the previous capacity is wrong.")
        (newSize)(oldSize)XEND;

    vaddr_t vaddr = oldSize + (vaddr_t)pool;
    vaddr_t const oldEnd = vaddr;

    Handle res = Vmm::AllocatePages(nullptr
        , newSize - oldSize
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::VasDescriptors
        , vaddr);

    if likely(res != HandleResult::Okay)
        return res;

    assert(vaddr == oldEnd);

    obj_ind_t const oldObjectCount = pool->Capacity;
    obj_ind_t const newObjectCount = (newSize - headerSize) / objectSize;

    uintptr_t cursor = (uintptr_t)pool + headerSize + oldObjectCount * objectSize;
    FreeObject * last = nullptr;

    if (pool->FreeCount > 0)
        last = pool->GetLastFreeObject(objectSize, headerSize);

    for (obj_ind_t i = oldObjectCount; i < newObjectCount; ++i, cursor += objectSize)
    {
        FreeObject * const obj = (FreeObject *)cursor;

        if unlikely(last == nullptr)
            pool->FirstFreeObject = i;
        else
            last->Next = i;

        last = obj;
    }

    //  After the loop is finished, `last` will point to the very last object
    //  in the pool. `newObjectCount - 1` will be the index of the last object.

    pool->LastFreeObject = (obj_ind_t)newObjectCount - 1;
    last->Next = obj_ind_invalid;

    pool->Capacity = newObjectCount;
    pool->FreeCount += newObjectCount - oldObjectCount;
    //  This is incremented because they could've been free objects prior to
    //  enlarging the pool.

    return HandleResult::Okay;
}
