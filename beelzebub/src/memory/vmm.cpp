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
#include <memory/pmm.hpp>
#include <memory/object_allocator_pools_heap.hpp>
#include <beel/interrupt.state.hpp>
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

/*  Statics  */

Spinlock<> Vmm::KernelHeapLock;

KernelVas Vmm::KVas;

/*  Page Management  */

Handle Vmm::HandlePageFault(Execution::Process * proc
    , uintptr_t const vaddr, PageFaultFlags const flags)
{
    //  Assumes interrupts are disabled upon call.

    if unlikely(0 != (flags & PageFaultFlags::Present))
        return HandleResult::Failed;
    //  Page is present. This means this is an access (write/execute) failure.

    if unlikely(!((vaddr >= Vmm::UserlandStart && vaddr <= Vmm::UserlandEnd)
               || (vaddr >= Vmm::KernelStart   && vaddr <= Vmm::KernelEnd  )))
        return HandleResult::Failed;
    //  The hit memory ought to be in either userland or the kernel heap.

    if (proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    Handle res = HandleResult::Okay;
    Memory::Vas * const vas = (vaddr < Vmm::UserlandEnd) ? &(proc->Vas) : &KVas;

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
                 || reg->Content == MemoryContent::Free)
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

    if unlikely(res != HandleResult::Okay)
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
                 && reg->Content == MemoryContent::Free)
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
         && (reg->Content == MemoryContent::Share
          || reg->Content == MemoryContent::Runtime))
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

/*  Allocation  */

Handle Vmm::AllocatePages(Process * proc, size_t const size
    , MemoryAllocationOptions const type
    , MemoryFlags const flags
    , MemoryContent content
    , uintptr_t & vaddr)
{
    if (proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    if (MemoryAllocationOptions::AllocateOnDemand == (type & MemoryAllocationOptions::AllocateOnDemand)
        || 0 == (type & MemoryAllocationOptions::Commit))
    {
        //  No AoD and no commit means it's just reserved.
        //  AoD and reserved are both very simple to handle.

        if (0 != (type & MemoryAllocationOptions::VirtualUser))
            return proc->Vas.Allocate(vaddr, size, flags, content, type);
        else
            return KVas.Allocate(vaddr, size, flags, content, type);
    }
    else if (0 != (type & MemoryAllocationOptions::Commit))
    {
        Handle res;             //  Intermediary result.
        vaddr_t ret = vaddr;    //  Just a quicker way...
        Spinlock<> * heapLock;

        if (0 != (type & MemoryAllocationOptions::VirtualUser))
        {
            res = proc->Vas.Allocate(ret, size, flags, content, type);

            heapLock = &(proc->LocalTablesLock);
        }
        else
        {
            res = KVas.Allocate(ret, size, flags, content, type);

            heapLock = &(Vmm::KernelHeapLock);
        }

        vaddr = ret;

        if unlikely(res != HandleResult::Okay)
            return res;

        InterruptGuard<> intGuard;
        //  Guard the rest of the scope from interrupts.

        LockGuard<Spinlock<> > heapLg {*heapLock};
        //  Note: this ain't flexible because heapLock ain't gonna be null.

        size_t offset;
        for (offset = 0; offset < size; offset += PageSize)
        {
            paddr_t const paddr = Pmm::AllocateFrame();

            if unlikely(paddr == nullpaddr)
                goto backtrack;

            res = Vmm::MapPage(proc, ret + offset, paddr
                , flags, MemoryMapOptions::NoLocking);

            if unlikely(res != HandleResult::Okay)
                goto backtrack;
        }

        if likely(0 != (type & MemoryAllocationOptions::VirtualUser))
            withWriteProtect (false)
                memset(reinterpret_cast<void *>(ret), 0xCA, size);

        return HandleResult::Okay;

    backtrack:
        //  So, the allocation failed. Now all the pages that were allocated
        //  need to be unmapped.

        res = Vmm::UnmapRange(proc, ret, offset, MemoryMapOptions::NoLocking);

        if unlikely(res != HandleResult::Okay && res != HandleResult::PageUnmapped)
            return res;

        return HandleResult::OutOfMemory;
    }

    FAIL("Odd memory allocation options given: %Xs", (size_t)type);
}

Handle Vmm::FreePages(Process * proc, uintptr_t const vaddr, size_t const size)
{
    vaddr_t const endAddr = vaddr + size;

    if unlikely(!((vaddr >= Vmm::UserlandStart && endAddr <= Vmm::UserlandEnd)
               || (vaddr >= Vmm::KernelStart   && endAddr <= Vmm::KernelEnd  )))
        return HandleResult::PageMapIllegalRange;
    //  Cannot use this to free memory from elsewhere.

    if unlikely(!Is4KiBAligned(vaddr))
        return HandleResult::AlignmentFailure;

    if (proc == nullptr) proc = likely(CpuDataSetUp) ? Cpu::GetProcess() : &BootstrapProcess;

    Handle res;

    if (endAddr <= Vmm::UserlandEnd)
        res = proc->Vas.Free(vaddr, size);
    else
        res = KVas.Free(vaddr, size);

    if unlikely(res != HandleResult::Okay)
        return res;
    
    return UnmapRange(proc, vaddr, size);
}
