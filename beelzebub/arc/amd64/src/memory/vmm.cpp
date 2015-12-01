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
#include <synchronization/spinlock_uninterruptible.hpp>
#include <system/cpu.hpp>
#include <kernel.hpp>

#include <string.h>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

/*  State Machine and Configuration  */

Atomic<bool> KernelHeapOverflown {false};

SpinlockUninterruptible<> VmmArc::KernelHeapLock;

bool VmmArc::Page1GB = false;
bool VmmArc::VmmArc::NX = false;

inline void Alienate(Process * const proc)
{
    Pml4 & pml4 = *(VmmArc::GetLocalPml4());

    pml4[VmmArc::AlienFractalIndex] = Pml4Entry(proc->PagingTable, true, true, false, VmmArc::NX);
}

/*  Statics  */

Atomic<vaddr_t> Vmm::KernelHeapCursor {VmmArc::KernelHeapStart};

/*  Initialization  */

Handle Vmm::Bootstrap(Process * const bootstrapProc)
{
    //  VmmArc::NX and VmmArc::Page1GB are set before executing this.

    if (VmmArc::NX)
        Cpu::EnableNxBit();

    PageDescriptor * desc = nullptr;
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
        paddr_t pml3_paddr = Domain0.PhysicalAllocator->AllocatePage(desc);

        memset((void *)pml3_paddr, 0, PageSize);
        //  Clear again.
        desc->IncrementReferenceCount();
        //  Increment reference count...

        newPml4[i] = Pml4Entry(pml3_paddr, true, true, true, false);
    }

    newPml4[VmmArc::LocalFractalIndex] = Pml4Entry(pml4_paddr, true, true, false, VmmArc::NX);
    newPml4[VmmArc::AlienFractalIndex] = newPml4[VmmArc::LocalFractalIndex];
    //  Very important note: the user-accessible bit is cleared.
    //  This means userland code will not be able to look at the fractal mapping.

    Vmm::Switch(nullptr, bootstrapProc);
    //  Activate, so pages can be mapped.

    //  Remapping PAS control structures.

    PageAllocationSpace * cur = Domain0.PhysicalAllocator->FirstSpace;
    bool pendingLinksMapping = true;
    vaddr_t curLoc = KernelHeapCursor; //  Used for serial allocation.
    Handle res; //  Temporary result.

    do
    {
        if ((vaddr_t)cur < VmmArc::HigherHalfStart && pendingLinksMapping)
        {
            //msg("Mapping links from %XP to %Xp. ", RoundDown((paddr_t)cur, PageSize), curLoc);

            res = Vmm::MapPage(bootstrapProc, curLoc, RoundDown((paddr_t)cur, PageSize)
                , MemoryFlags::Global | MemoryFlags::Writable, PageDescriptor::Invalid
                , false);
            //  Global because it's shared by processes, and writable for hotplug.

            ASSERT(res.IsOkayResult()
                , "Failed to map links between allocation spaces: %H"
                , res);
            //  Failure is fatal.

            Domain0.PhysicalAllocator->RemapLinks(RoundDown((vaddr_t)cur, PageSize), curLoc);
            //  Do the actual remapping.

            pendingLinksMapping = false;
            //  One page is the maximum.

            curLoc += PageSize;
            //  Increment the current location.
        }

        paddr_t const pasStart = cur->GetMemoryStart();
        size_t const controlStructuresSize = (cur->GetAllocationStart() - pasStart);
        //  Size of control pages.

        if (curLoc + controlStructuresSize > VmmArc::KernelHeapEnd)
            break;
        //  Well, the maximum is reached! Like this will ever happen...

        for (size_t i = 0; i < controlStructuresSize; i += PageSize)
        {
            res = Vmm::MapPage(bootstrapProc, curLoc + i, pasStart + i
                , MemoryFlags::Global | MemoryFlags::Writable, PageDescriptor::Invalid
                , false);

            ASSERT(res.IsOkayResult()
                , "Failed to map page #%u8 (%Xp to %XP): %H"
                , i / PageSize, curLoc + i, pasStart + i, res);
            //  Failure is fatal.
        }

        cur->RemapControlStructures(curLoc);
        //  Self-documented function name.

        curLoc += controlStructuresSize;

    } while ((cur = cur->Next) != nullptr);

    KernelHeapCursor = curLoc;

    Pml4 & pml4 = *(VmmArc::GetLocalPml4());

    for (uint16_t i = 0; i < 256; ++i)
        pml4[i] = Pml4Entry();
    //  Getting rid of those naughty identity maps.

    Vmm::Switch(nullptr, bootstrapProc);
    //  Re-activate, to flush the identity maps.

    return Handle(HandleResult::Okay);
}

Handle Vmm::Initialize(Process * const proc)
{
    CpuInstructions::InvalidateTlb(VmmArc::GetAlienPml4());

    // msg("<[ CLONE SOURCE PML4 ]>%n");
    // PrintToDebugTerminal(*VmmArc::GetLocalPml4());
    // msg("%n%n");

    PageDescriptor * desc;

    paddr_t const pml4_paddr = proc->PagingTable
        = Domain0.PhysicalAllocator->AllocatePage(desc);

    if (pml4_paddr == nullpaddr)
        return HandleResult::OutOfMemory;

    desc->IncrementReferenceCount();
    //  Do the good deed.

    SpinlockUninterruptible<> * alienLock = nullptr;

    if (CpuDataSetUp)
        alienLock = &(Cpu::GetData()->ActiveThread->Owner->AlienPagingTablesLock);

    LockGuardFlexible<SpinlockUninterruptible<> > pml4Lg {*alienLock};

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

    // msg("<[ CLONE TARGET PML4 ]>%n");
    // PrintToDebugTerminal(*VmmArc::GetAlienPml4());
    // msg("%n%n");

    if (CpuDataSetUp)
        Cpu::GetData()->LastAlienPml4 = pml4_paddr;

    return HandleResult::Okay;
}

/*  Activation and Status  */

Handle Vmm::Switch(Process * const oldProc, Process * const newProc)
{
    Cr3 const newVal = Cr3(newProc->PagingTable, false, false);

    Cpu::SetCr3(newVal);

    return HandleResult::Okay;
}

bool Vmm::IsActive(Process * const proc)
{
    Pml4 & pml4 = *(VmmArc::GetLocalPml4());

    return pml4[VmmArc::LocalFractalIndex].GetPml3Address() == proc->PagingTable;
}

bool Vmm::IsAlien(Process * const proc)
{
    Pml4 & pml4 = *(VmmArc::GetLocalPml4());

    return pml4[VmmArc::AlienFractalIndex].GetPml3Address() == proc->PagingTable;
}

/*  Page Management  */

template<typename cbk_t>
__hot inline Handle TryTranslate(Process * const proc, uintptr_t const vaddr
             , cbk_t cbk, bool const lock)
{
    if unlikely((vaddr >= VmmArc::FractalStart && vaddr < VmmArc::FractalEnd     )
             || (vaddr >= VmmArc::LowerHalfEnd && vaddr < VmmArc::HigherHalfStart))
        return HandleResult::PageMapIllegalRange;

    //  No alignment check will be performed here.

    bool const nonLocal = (vaddr < VmmArc::LowerHalfEnd) && !Vmm::IsActive(proc);
    uint16_t ind;   //  Used to hold the current index.

    Pml4 * pml4p; Pml3 * pml3p; Pml2 * pml2p; Pml1 * pml1p;

    SpinlockUninterruptible<> * alienLock = nullptr, * heapLock = nullptr;

    {   //  Lock-guarded.

        if (nonLocal && CpuDataSetUp)
            alienLock = &(Cpu::GetData()->ActiveThread->Owner->AlienPagingTablesLock);

        LockGuardFlexible<SpinlockUninterruptible<> > pml4Lg {*alienLock};

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

        {   //  Lock-guarded.

            if (lock) heapLock = (vaddr < VmmArc::LowerHalfEnd ? &(proc->UserHeapLock) : &(VmmArc::KernelHeapLock));

            LockGuardFlexible<SpinlockUninterruptible<> > heapLg {*heapLock};

            Pml4 & pml4 = *pml4p;
            ind = VmmArc::GetPml4Index(vaddr);

            if unlikely(!pml4[ind].GetPresent())
                return HandleResult::PageUnmapped;

            Pml3 & pml3 = *pml3p;
            ind = VmmArc::GetPml3Index(vaddr);

            if unlikely(!pml3[ind].GetPresent())
                return HandleResult::PageUnmapped;
            
            Pml2 & pml2 = *pml2p;
            ind = VmmArc::GetPml2Index(vaddr);

            if unlikely(!pml2[ind].GetPresent())
                return HandleResult::PageUnmapped;
            
            Pml1 & pml1 = *pml1p;
            ind = VmmArc::GetPml1Index(vaddr);

            return cbk(pml1.Entries + ind);
        }
    }
}

Handle Vmm::MapPage(Process * const proc, uintptr_t const vaddr, paddr_t const paddr
             , MemoryFlags const flags, PageDescriptor * desc, bool const lock)
{
    if unlikely((vaddr >= VmmArc::FractalStart && vaddr < VmmArc::FractalEnd     )
             || (vaddr >= VmmArc::LowerHalfEnd && vaddr < VmmArc::HigherHalfStart))
        return HandleResult::PageMapIllegalRange;

    if unlikely((vaddr & (PageSize - 1)) != 0 || (paddr & (PageSize - 1)) != 0)
        return HandleResult::PageUnaligned;

    PageAllocator * alloc = Domain0.PhysicalAllocator;

    //  TODO: NUMA settings, and get the domain from CPU data.

#define ALLOCATE_TABLE(N, M)                                                                    \
    if unlikely(!MCATS(pml, M)[ind].GetPresent())                                               \
    {                                                                                           \
        PageDescriptor * desc;                                                                  \
        paddr_t const MCATS(newPml, N) = alloc->AllocatePage(desc);                             \
                                                                                                \
        if (MCATS(newPml, N) == nullpaddr)                                                      \
            return Handle(HandleResult::OutOfMemory);                                           \
                                                                                                \
        desc->IncrementReferenceCount();                                                        \
                                                                                                \
        MCATS(pml, M)[ind] = MCATS(Pml, M, Entry)(MCATS(newPml, N), true, true, true, false);   \
        /*  Present, writable, user-accessible, executable.  */                                 \
                                                                                                \
        memset(MCATS(pml, N, p), 0, PageSize);                                                  \
    }

    bool const nonLocal = (vaddr < VmmArc::LowerHalfEnd) && !Vmm::IsActive(proc);
    uint16_t ind;   //  Used to hold the current index.

    Pml4 * pml4p; Pml3 * pml3p; Pml2 * pml2p; Pml1 * pml1p;

    SpinlockUninterruptible<> * alienLock = nullptr, * heapLock = nullptr;

    {   //  Lock-guarded.

        if (nonLocal && CpuDataSetUp)
            alienLock = &(Cpu::GetData()->ActiveThread->Owner->AlienPagingTablesLock);

        LockGuardFlexible<SpinlockUninterruptible<> > pml4Lg {*alienLock};

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

        {   //  Lock-guarded.

            if (lock)
                heapLock = (vaddr < VmmArc::LowerHalfEnd
                    ? &(proc->UserHeapLock)
                    : &(VmmArc::KernelHeapLock));

            LockGuardFlexible<SpinlockUninterruptible<> > heapLg {*heapLock};

            Pml4 & pml4 = *pml4p;
            ind = VmmArc::GetPml4Index(vaddr);

            ALLOCATE_TABLE(3, 4);

            Pml3 & pml3 = *pml3p;
            ind = VmmArc::GetPml3Index(vaddr);

            ALLOCATE_TABLE(2, 3);
            
            Pml2 & pml2 = *pml2p;
            ind = VmmArc::GetPml2Index(vaddr);

            ALLOCATE_TABLE(1, 2);
            
            Pml1 & pml1 = *pml1p;
            ind = VmmArc::GetPml1Index(vaddr);

            if unlikely(pml1[ind].GetPresent())
                return HandleResult::PageMapped;

            //  TODO: Check page tags (reserved, allocated on demand, etc.)

            pml1[ind] = Pml1Entry(paddr, true
                , 0 != (flags & MemoryFlags::Writable)
                , 0 != (flags & MemoryFlags::Userland)
                , 0 != (flags & MemoryFlags::Global)
                , 0 == (flags & MemoryFlags::Executable) && VmmArc::NX);
            //  Present, writable, user-accessible, global, executable.
        }
    }

#undef ALLOCATE_TABLE

    if likely(PageDescriptor::IsValid(desc))
    {
        if likely(desc != nullptr)
            desc->IncrementReferenceCount();
        else if (alloc->TryGetPageDescriptor(paddr, desc))
            desc->IncrementReferenceCount();
        //  The page still may not be in an allocation space.
    }

    return HandleResult::Okay;
}

Handle Vmm::UnmapPage(Process * const proc, uintptr_t const vaddr
    , paddr_t & paddr, PageDescriptor * & desc, bool const lock)
{
    PageAllocator * alloc = Domain0.PhysicalAllocator;

    //  TODO: NUMA settings, and get the domain from CPU data.

    Handle res = TryTranslate(proc, vaddr, [&paddr](Pml1Entry * pE)
    {
        if likely(pE->GetPresent())
        {
            paddr = pE->GetAddress();
            *pE = Pml1Entry();
            //  Null.

            return HandleResult::Okay;
        }
        else
        {
            paddr = 0;

            return HandleResult::PageUnmapped;
        }
    }, lock);

    if (!res.IsOkayResult())
    {
        desc = nullptr;

        return res;
    }

    //  The rest is done outside of the lambda because locks are unnecessary.

    Vmm::InvalidatePage(proc, vaddr, true);

    if (alloc->TryGetPageDescriptor(paddr, desc))
    {
        auto refcnt = desc->DecrementReferenceCount();

        if (refcnt == 0)
            alloc->FreePageAtAddress(paddr);
    }

    //  TODO: The page may be in another domain's allocator.
    //  That needs to be handled!

    return HandleResult::Okay;
}

Handle Vmm::InvalidatePage(Process * const proc, uintptr_t const vaddr
    , bool const broadcast)
{
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

/*  Allocation  */

Handle Vmm::AllocatePages(Process * const proc, size_t const count
    , MemoryAllocationOptions const type, MemoryFlags const flags, uintptr_t & vaddr)
{
    if (MemoryAllocationOptions::AllocateOnDemand == (type & MemoryAllocationOptions::AllocateOnDemand))
    {
        return HandleResult::NotImplemented;
        //  Not possible yet.
    }
    else if (0 != (type & MemoryAllocationOptions::Commit))
    {
        size_t const  lowerOffset = (0 != (type & MemoryAllocationOptions::GuardLow))
            ? PageSize : 0;
        size_t const higherOffset = (0 != (type & MemoryAllocationOptions::GuardHigh))
            ? PageSize : 0;

        SpinlockUninterruptible<> * heapLock;

        vaddr_t ret;

        if (0 != (type & MemoryAllocationOptions::VirtualUser))
        {
            if (proc->UserHeapOverflown)
                return HandleResult::UnsupportedOperation;
            //  Not supported yet.
            
            //  TODO: Add VAS.
            
            ret = proc->UserHeapCursor.FetchAdd(count * PageSize + lowerOffset + higherOffset);

            if (ret > VmmArc::KernelHeapEnd)
            {
                proc->UserHeapCursor.CmpXchgStrong(ret, ret - VmmArc::KernelHeapLength);
                proc->UserHeapOverflown = true;

                return HandleResult::UnsupportedOperation;
                //  Not supported yet.
            }

            vaddr = ret + lowerOffset;

            heapLock = &(proc->UserHeapLock);
        }
        else
        {
            if (KernelHeapOverflown)
                return HandleResult::UnsupportedOperation;
            //  Not supported yet.

            ret = KernelHeapCursor.FetchAdd(count * PageSize + lowerOffset + higherOffset);

            if (ret > VmmArc::KernelHeapEnd)
            {
                KernelHeapCursor.CmpXchgStrong(ret, ret - VmmArc::KernelHeapLength);
                KernelHeapOverflown = true;

                return HandleResult::UnsupportedOperation;
                //  Not supported yet.
            }

            vaddr = ret + lowerOffset;

            heapLock = &(VmmArc::KernelHeapLock);
        }

        Handle res;
        PageDescriptor * desc;
        //  Intermediary results.

        PageAllocator * alloc = Domain0.PhysicalAllocator;

        if (CpuDataSetUp)
            alloc = Cpu::GetData()->DomainDescriptor->PhysicalAllocator;

        size_t const size = count * PageSize;
        bool allocSucceeded = true;

        LockGuard<SpinlockUninterruptible<> > heapLg {*heapLock};
        //  Note: this ain't flexible because heapLock ain't gonna be null.

        size_t offset;
        for (offset = 0; offset < size; offset += PageSize)
        {
            paddr_t const paddr = alloc->AllocatePage(desc);
            
            if (paddr == nullpaddr) { allocSucceeded = false; break; }

            res = Vmm::MapPage(proc, ret + lowerOffset + offset, paddr
                , flags, desc, false);

            if (!res.IsOkayResult()) { allocSucceeded = false; break; }
        }

        if likely(allocSucceeded)
            return HandleResult::Okay;
        else
        {
            //  So, the allocation failed. Now all the pages that were allocated
            //  need to be unmapped.

            do
            {
                res = Vmm::UnmapPage(proc, ret + lowerOffset + offset, false);

                if unlikely(!res.IsOkayResult() && !res.IsResult(HandleResult::PageUnmapped))
                    return res;

                offset -= PageSize;
            } while (offset > 0);

            return HandleResult::OutOfMemory;
        }
    }
    else // Means it'll just be reserveed.
    {
        size_t const  lowerOffset = (0 != (type & MemoryAllocationOptions::GuardLow))
            ? PageSize : 0;
        size_t const higherOffset = (0 != (type & MemoryAllocationOptions::GuardHigh))
            ? PageSize : 0;

        if (0 != (type & MemoryAllocationOptions::VirtualUser))
        {
            //  TODO: Scrap this, for proper VAS.

            if (proc->UserHeapOverflown)
            {
                return HandleResult::UnsupportedOperation;
                //  Not supported yet.
            }

            vaddr_t ret = proc->UserHeapCursor.FetchAdd(count * PageSize + lowerOffset + higherOffset);

            if (ret > VmmArc::KernelHeapEnd)
            {
                proc->UserHeapCursor.CmpXchgStrong(ret, ret - VmmArc::KernelHeapLength);
                proc->UserHeapOverflown = true;

                vaddr = nullvaddr;
                return HandleResult::UnsupportedOperation;
                //  Not supported yet.
            }

            vaddr = ret + lowerOffset;

            return HandleResult::Okay;
        }
        else
        {
            if (KernelHeapOverflown)
            {
                return HandleResult::UnsupportedOperation;
                //  Not supported yet.
            }

            vaddr_t ret = KernelHeapCursor.FetchAdd(count * PageSize + lowerOffset + higherOffset);

            if (ret > VmmArc::KernelHeapEnd)
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

}

/*  Flags  */

Handle Vmm::GetPageFlags(Process * const proc, uintptr_t const vaddr
    , MemoryFlags & flags, bool const lock)
{
    Handle res = TryTranslate(proc, vaddr, [&flags](Pml1Entry * pE)
    {
        if likely(pE->GetPresent())
        {
            Pml1Entry const e = *pE;
            MemoryFlags f = MemoryFlags::None;

            if (  e.GetGlobal())            f |= MemoryFlags::Global;
            if (  e.GetUserland())          f |= MemoryFlags::Userland;
            if (  e.GetWritable())          f |= MemoryFlags::Writable;
            if (!(e.GetXd() && VmmArc::NX)) f |= MemoryFlags::Executable;

            flags = f;

            return HandleResult::Okay;
        }
        else
        {
            flags = MemoryFlags::None;

            return HandleResult::PageUnmapped;
        }
    }, lock);

    if (!res.IsOkayResult())
        return res;

    return Vmm::InvalidatePage(proc, vaddr, true);
}

Handle Vmm::SetPageFlags(Process * const proc, uintptr_t const vaddr
    , MemoryFlags const flags, bool const lock)
{
    Handle res = TryTranslate(proc, vaddr, [flags](Pml1Entry * pE)
    {
        if likely(pE->GetPresent())
        {
            Pml1Entry e = *pE;
    
            e.SetGlobal( ((MemoryFlags::Global     & flags) != 0))
            .SetUserland(((MemoryFlags::Userland   & flags) != 0))
            .SetWritable(((MemoryFlags::Writable   & flags) != 0))
            .SetXd( VmmArc::NX & ((MemoryFlags::Executable & flags) == 0));

            *pE = e;

            return HandleResult::Okay;
        }
        else
            return HandleResult::PageUnmapped;
    }, lock);

    if (!res.IsOkayResult())
        return res;

    return Vmm::InvalidatePage(proc, vaddr, true);
}
