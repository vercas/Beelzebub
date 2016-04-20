/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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

#include <terminals/cache_pools_heap.hpp>
#include <memory/vmm.hpp>
#include <system/cpu.hpp>
#include <kernel.hpp>
#include <entry.h>

#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;

static __noinline Handle GetKernelHeapPages(size_t const pageCount, uintptr_t & address)
{
    vaddr_t vaddr = nullvaddr;

    Handle res = Vmm::AllocatePages(
        CpuDataSetUp ? Cpu::GetProcess() : &BootstrapProcess
        , pageCount, MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global | MemoryFlags::Writable, vaddr);

    address = (uintptr_t)vaddr;

    return res;
}

Handle Terminals::AcquireCharPoolInKernelHeap(size_t minSize
                                            , size_t headerSize
                                            , CharPool * & result)
{
    assert(headerSize >= sizeof(CharPool)
        , "The given header size (%us) apprats to be lower than the size of an "
          "actual pool struct (%us)..?%n"
        , headerSize, sizeof(CharPool));

    size_t const pageCount = RoundUp(minSize + 1 + headerSize, PageSize) / PageSize;
    uintptr_t addr = 0;

    Handle res = GetKernelHeapPages(pageCount, addr);

    if (!res.IsOkayResult())
        return res;

    CharPool * pool = reinterpret_cast<CharPool *>(addr);
    //  I use a local variable here so `result` isn't dereferenced every time.

    size_t const capacity = (pageCount * PageSize) - headerSize - 1;
    //  That -1 is for a null terminator!

    new (pool) CharPool((uint32_t)capacity);
    //  Construct in place to initialize the fields.

    memset(const_cast<char *>(pool->GetString()), 0, capacity + 1);
    //  Fill it with zeros, so any string appended into this pool will have a
    //  null terminator.

    result = pool;

    return HandleResult::Okay;
}

Handle Terminals::EnlargeCharPoolInKernelHeap(size_t minSize
                                            , size_t headerSize
                                            , CharPool * pool)
{
    size_t const oldPageCount = RoundUp(pool->Capacity + 1 + headerSize, PageSize) / PageSize;
    size_t newPageCount = RoundUp(pool->Capacity + 1 + minSize + headerSize, PageSize) / PageSize;

    assert(newPageCount > oldPageCount
        , "New page count (%us) should be larger than the old page count (%us) "
          "of a pool that needs enlarging!%nIt appears that the previous capacity"
          "is wrong.%n"
        , newPageCount, oldPageCount);

    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    vaddr_t const vaddr = oldPageCount * PageSize + (vaddr_t)pool;

    vaddr_t oldPoolEnd = vaddr;
    vaddr_t newPoolEnd = vaddr + (newPageCount - oldPageCount) * PageSize;

    // if unlikely(newPoolEnd > MemoryManagerAmd64::KernelHeapEnd)
    // {
    //     newPageCount -= (newPoolEnd - MemoryManagerAmd64::KernelHeapEnd) / PageSize;
    //     newPoolEnd = vaddr + (newPageCount - oldPageCount) * PageSize;
    // }

    bool const swapped = Vmm::KernelHeapCursor.CmpXchgStrong(oldPoolEnd, newPoolEnd);

    if (!swapped)
        return HandleResult::PageMapped;
    //  It is possible that something else has already reserved memory here.
    //  TODO: Check for actual free pages.

    size_t curPageCount = oldPageCount;

    for (size_t i = 0; curPageCount < newPageCount; ++i, ++curPageCount)
    {
        paddr_t const paddr =
            (CpuDataSetUp ? Cpu::GetData()->DomainDescriptor : &Domain0)
            ->PhysicalAllocator->AllocatePage(desc);
        //  Test page.

        assert_or(paddr != nullpaddr && desc != nullptr
            , "Unable to allocate physical page #%us for extending object pool "
              "%Xp (%us, %us, %us, %us)!"
            , i, pool
            , minSize, headerSize, oldPageCount, newPageCount)
        {
            break;
        }

        res = Vmm::MapPage(
            CpuDataSetUp ? Cpu::GetProcess() : &BootstrapProcess,
            vaddr + i * PageSize,
            paddr,
            MemoryFlags::Global | MemoryFlags::Writable,
            desc
        );

        assert_or(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP; #%us) for extending object pool "
              "%Xp (%us, %us, %us, %us): %H."
            , vaddr + i * PageSize, paddr, i, pool
            , minSize, headerSize, oldPageCount, newPageCount
            , res)
        {
            (CpuDataSetUp ? Cpu::GetData()->DomainDescriptor : &Domain0)
                ->PhysicalAllocator->FreePageAtAddress(paddr);
            //  This should succeed.

            break;
        }
    }

    if (curPageCount == oldPageCount)
        return res;
    //  Nothing was allocated.

    uint32_t const oldSize = pool->Capacity;
    pool->Capacity = (curPageCount * PageSize) - headerSize - 1;

    memset(const_cast<char *>(pool->GetString()) + oldSize, 0, pool->Capacity - oldSize);
    //  Now anything appended will still yield a valid string.

    return HandleResult::Okay;
}

Handle Terminals::ReleaseCharPoolFromKernelHeap(size_t headerSize
                                              , CharPool * pool)
{
    //  A nice procedure here is to unmap the pool's pages one-by-one, starting
    //  from the highest. The kernel heap cursor will be pulled back if possible.

    Handle res;
    PageDescriptor * desc = nullptr;

    bool decrementedHeapCursor = true;
    //  Initial value is for simplifying the algorithm below.

    size_t const pageCount = RoundUp(pool->Capacity + 1 + headerSize, PageSize) / PageSize;

    vaddr_t vaddr = (vaddr_t)reinterpret_cast<uintptr_t>(pool) + (pageCount - 1) * PageSize;
    size_t i = pageCount;

    for (/* nothing */; i > 0; --i, vaddr -= PageSize)
    {
        res = Vmm::UnmapPage(
            CpuDataSetUp ? Cpu::GetProcess() : &BootstrapProcess,
            vaddr,
            desc
        );

        assert_or(res.IsOkayResult()
            , "Failed to unmap page #%us from pool %Xp.%n"
            , i, pool)
        {
            break;

            //  The rest of the function will attempt to adapt.
        }

        vaddr_t expectedCursor = vaddr + PageSize;

        if (decrementedHeapCursor)
            decrementedHeapCursor = Vmm::KernelHeapCursor.CmpXchgStrong(expectedCursor, vaddr);
    }

    //  TODO: Salvage pool when it failed to remove all pages.
    //if (i > 0 && i < pageCount)

    return i < pageCount
        ? HandleResult::Okay // At least one page was freed so the pool needs removal...
        : HandleResult::UnsupportedOperation;
}
