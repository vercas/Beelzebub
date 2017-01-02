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

#include <sys/mman.h>
#include "memory/vmm.hpp"

using namespace Beelzebub;
using namespace Beelzebub::Memory;

void * mmap(void * const addr, size_t const length, int const prot
    , int const flags, int const fd, off_t const offset)
{
    (void)fd;
    (void)offset;

    Handle res;
    MemoryFlags f = MemoryFlags::Global;
    MemoryAllocationOptions o = MemoryAllocationOptions::VirtualKernelHeap;
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(addr);

    // if unlikely(fd != -1)
    //     goto failure;
    if unlikely(0 == (flags & MAP_ANONYMOUS))
        goto failure;
    if unlikely(0 != (flags & MAP_SHARED))
        goto failure;
    if unlikely(length % PageSize != 0)
        goto failure;
    if unlikely(vaddr % PageSize != 0)
        goto failure;

    if (0 != (prot & PROT_WRITE))
        f |= MemoryFlags::Writable;
    if (0 != (prot & PROT_EXEC))
        f |= MemoryFlags::Executable;

    if (0 != (prot & PROT_READ))
        o |= MemoryAllocationOptions::AllocateOnDemand;
    else
    {
        FAIL("Unsupported!");

        o |= MemoryAllocationOptions::Reserve;
    }

    res = Vmm::AllocatePages(nullptr, length, o, f, MemoryContent::Generic, vaddr);

    if unlikely(res != HandleResult::Okay)
        goto failure;

    return reinterpret_cast<void *>(vaddr);

failure:
    return MAP_FAILED;
}

int munmap(void * addr, size_t length)
{
    MSG_("munmap %Xp %Xs%n", addr, length);

    Handle res = Vmm::FreePages(nullptr, reinterpret_cast<uintptr_t>(addr), length);

    if unlikely(res != HandleResult::Okay)
    {
        return -1;
    }

    return 0;
}

void * mremap(void * old_address, size_t old_size, size_t new_size, int flags, ...)
{
    (void)old_address;
    (void)old_size;
    (void)new_size;
    (void)flags;

    FAIL();

    return nullptr;
}

