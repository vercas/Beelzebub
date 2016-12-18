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

void Memory::FillPool(ObjectPoolBase volatile * volatile pool
                    , size_t const objectSize
                    , size_t const headerSize
                    , obj_ind_t const objectCount)
{
    ASSUME(pool != nullptr);
    ASSUME(objectCount > 0);

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

    ASSUME(last != nullptr);

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
        , "The given header size apprats to be lower than the size of an "
          "actual pool struct..?")
        (headerSize)(sizeof(ObjectPoolBase));

    size_t const size = RoundUp(objectSize * minimumObjects + headerSize, PageSize);
    uintptr_t addr = 0;

    Handle res = Vmm::AllocatePages(nullptr
        , size
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

    size_t const objectCount = (size - headerSize) / objectSize;
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
    size_t const oldSize = RoundUp(objectSize * pool->Capacity + headerSize, PageSize);
    size_t const newSize = RoundUp(objectSize * (pool->Capacity + minimumExtraObjects) + headerSize, PageSize);

    ASSERT(newSize > oldSize
        , "New size should be larger than the old size of a pool that needs enlarging!%n"
          "It appears that the previous capacity is wrong.")
        (newSize)(oldSize);

    vaddr_t vaddr = oldSize + (vaddr_t)pool;
    vaddr_t const oldEnd = vaddr;

    Handle res = Vmm::AllocatePages(nullptr
        , newSize - oldSize
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::Generic
        , vaddr);

    if likely(!res.IsOkayResult())
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

Handle Memory::ReleasePoolFromKernelHeap(size_t objectSize
                                       , size_t headerSize
                                       , ObjectPoolBase * pool)
{
    Handle res = Vmm::FreePages(nullptr
        , reinterpret_cast<uintptr_t>(pool)
        , RoundUp(objectSize * pool->Capacity + headerSize, PageSize));

    assert(res.IsOkayResult(), "Failed to unmap object pool.")
        ("pool", (void *)pool)
        ("size", RoundUp(objectSize * pool->Capacity + headerSize, PageSize))
        (res);

    return res;
}
