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

#include <string.h>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;

Handle Terminals::AcquireCharPoolInKernelHeap(size_t minSize
                                            , size_t headerSize
                                            , CharPool * & result)
{
    assert(headerSize >= sizeof(CharPool)
        , "The given header size (%us) apprats to be lower than the size of an "
          "actual pool struct (%us)..?%n"
        , headerSize, sizeof(CharPool));

    vsize_t const size = RoundUp(vsize_t(minSize + 1 + headerSize), PageSize);
    vaddr_t addr = nullvaddr;

    Handle res = Vmm::AllocatePages(nullptr
        , size
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::Generic
        , addr);

    if (!res.IsOkayResult())
        return res;

    CharPool * pool = reinterpret_cast<CharPool *>(addr.Value);
    //  I use a local variable here so `result` isn't dereferenced every time.

    size_t const capacity = size.Value - headerSize - 1;
    //  That -1 is for a null terminator!

    new (pool) CharPool((uint32_t)capacity);
    //  Construct in place to initialize the fields.

    ::memset(const_cast<char *>(pool->GetString()), 0, capacity + 1);
    //  Fill it with zeros, so any string appended into this pool will have a
    //  null terminator.

    result = pool;

    return HandleResult::Okay;
}

Handle Terminals::EnlargeCharPoolInKernelHeap(size_t minSize
                                            , size_t headerSize
                                            , CharPool * pool)
{
    vsize_t const oldSize = RoundUp(vsize_t(pool->Capacity + 1 + headerSize          ), PageSize);
    vsize_t const newSize = RoundUp(vsize_t(pool->Capacity + 1 + headerSize + minSize), PageSize);

    assert(newSize > oldSize
        , "New size should be larger than the old size of a pool that needs enlarging!%n"
          "It appears that the previous capacity is wrong.%n")
        (newSize)(oldSize);

    vaddr_t vaddr = (vaddr_t)pool + oldSize;

    Handle res = Vmm::AllocatePages(nullptr
        , newSize - oldSize
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::Generic
        , vaddr);

    if likely(!res.IsOkayResult())
        return res;

    uint32_t const oldCap = pool->Capacity;
    pool->Capacity = newSize.Value - headerSize - 1;

    ::memset(const_cast<char *>(pool->GetString()) + oldCap, 0, pool->Capacity - oldCap);
    //  Now anything appended will still yield a valid string.

    return HandleResult::Okay;
}

Handle Terminals::ReleaseCharPoolFromKernelHeap(size_t headerSize
                                              , CharPool * pool)
{
    Handle res = Vmm::FreePages(nullptr
        , vaddr_t(pool)
        , RoundUp(vsize_t(pool->Capacity + 1 + headerSize), PageSize));

    assert(res.IsOkayResult(), "Failed to unmap object pool.")("pool", (void *)pool)(res);

    return res;
}
