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

#include <memory/object_allocator_pools_heap.hpp>
#include <memory/vmm.hpp>
#include <system/cpu.hpp>
#include <kernel.hpp>
#include <entry.h>

#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;

static void FillPool(ObjectPoolBase volatile * volatile pool
                              , size_t const objectSize
                              , size_t const headerSize
                              , obj_ind_t const objectCount)
{
    pool->Capacity = objectCount;
    pool->FreeCount = objectCount;

    COMPILER_MEMORY_BARRIER();

    uintptr_t cursor = (uintptr_t)pool + headerSize;
    FreeObject * last = nullptr;

    for (obj_ind_t i = 0; i < objectCount; ++i, cursor += objectSize)
    {
        //  Note: `cursor` is incremented in the loop construct.
        //  This loops just set the previous object's `Next` pointer to the
        //  index of the current object. If there is no previous object,
        //  the pool's first object is set to the index of the current object.

        FreeObject * const obj = (FreeObject *)cursor;

        if unlikely(last == nullptr)
            pool->FirstFreeObject = i;
        else
            last->Next = i;

        last = obj;
    }

    //  After the loop is finished, `last` will point to the very last object
    //  in the pool. `pool->Capacity - 1` will be the index of the last object.

    pool->LastFreeObject = objectCount - 1;
    last->Next = obj_ind_invalid;

    COMPILER_MEMORY_BARRIER();
}

Handle Memory::AcquirePoolInKernelHeap(size_t objectSize
                                     , size_t headerSize
                                     , size_t minimumObjects
                                     , ObjectPoolBase * & result)
{
    assert(headerSize >= sizeof(ObjectPoolBase)
        , "The given header size (%us) apprats to be lower than the size of an "
          "actual pool struct (%us)..?%n"
        , headerSize, sizeof(ObjectPoolBase));

    size_t const pageCount = RoundUp(objectSize * minimumObjects + headerSize, PageSize) / PageSize;
    uintptr_t addr = 0;

    Handle res = Vmm::AllocatePages(nullptr
        , pageCount
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::Generic
        , addr);

    if (!res.IsOkayResult())
        return res;

    ObjectPoolBase volatile * volatile pool = (ObjectPoolBase *)(uintptr_t)addr;
    //  I use a local variable here so `result` isn't dereferenced every time.

    new (const_cast<ObjectPoolBase *>(pool)) ObjectPoolBase();
    //  Construct in place to initialize the fields.

    size_t const objectCount = ((pageCount * PageSize) - headerSize) / objectSize;
    //  TODO: Get rid of this division and make the loop below stop when the
    //  cursor reaches the end of the page(s).

    FillPool(pool, objectSize, headerSize, (obj_ind_t)objectCount);

    //  The pool was constructed in place, so the rest of the fields should
    //  be in a good state.

    result = const_cast<ObjectPoolBase *>(pool);

    return HandleResult::Okay;
}

Handle Memory::EnlargePoolInKernelHeap(size_t objectSize
                                     , size_t headerSize
                                     , size_t minimumExtraObjects
                                     , ObjectPoolBase * pool)
{
    size_t const oldPageCount = RoundUp(objectSize * pool->Capacity + headerSize, PageSize) / PageSize;
    size_t newPageCount = RoundUp(objectSize * (pool->Capacity + minimumExtraObjects) + headerSize, PageSize) / PageSize;

    ASSERT(newPageCount > oldPageCount
        , "New page count (%us) should be larger than the old page count (%us) "
          "of a pool that needs enlarging!%nIt appears that the previous capacity"
          "is wrong.%n"
        , newPageCount, oldPageCount);

    vaddr_t vaddr = oldPageCount * PageSize + (vaddr_t)pool;

    Handle res = Vmm::AllocatePages(nullptr
        , newPageCount - oldPageCount
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::Generic
        , vaddr);

    if likely(!res.IsOkayResult())
        return res;

    obj_ind_t const oldObjectCount = pool->Capacity;
    obj_ind_t const newObjectCount = ((newPageCount * PageSize) - headerSize) / objectSize;

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

Handle Memory::ReleasePoolFromKernelHeap(size_t objectSize
                                       , size_t headerSize
                                       , ObjectPoolBase * pool)
{
    //  A nice procedure here is to unmap the pool's pages one-by-one, starting
    //  from the highest. The kernel heap cursor will be pulled back if possible.

    Handle res;

    bool decrementedHeapCursor = true;
    //  Initial value is for simplifying the algorithm below.

    size_t const pageCount = RoundUp(objectSize * pool->Capacity + headerSize, PageSize) / PageSize;

    vaddr_t vaddr = (vaddr_t)pool + (pageCount - 1) * PageSize;
    size_t i = pageCount;

    for (/* nothing */; i > 0; --i, vaddr -= PageSize)
    {
        res = Vmm::UnmapPage(nullptr, vaddr);

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
