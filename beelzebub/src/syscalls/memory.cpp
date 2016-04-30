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

#include <syscalls/memory.h>
#include <memory/vmm.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Syscalls;

handle_t Syscalls::MemoryRequest(uintptr_t addr, size_t size, mem_req_opts_t opts)
{
    if unlikely(addr != 0 && (addr < Vmm::UserlandStart || addr >= Vmm::UserlandEnd))
        return HandleResult::ArgumentOutOfRange;

    if unlikely(addr % PageSize != 0 || size % PageSize != 0)
        return HandleResult::AlignmentFailure;

    uintptr_t const end = addr + size;

    if unlikely(end < addr || end > Vmm::UserlandEnd)
        return HandleResult::ArgumentOutOfRange;

    MemoryAllocationOptions type = MemoryAllocationOptions::VirtualUser;

    if (0 != (opts & mem_req_opts_t::Commit))
        type |= MemoryAllocationOptions::Commit;
    else if (0 == (opts & mem_req_opts_t::Reserve))
        type |= MemoryAllocationOptions::AllocateOnDemand;

    if (0 != (opts & mem_req_opts_t::ThreadStack))
        type |= MemoryAllocationOptions::ThreadStack;

    if (0 != (opts & mem_req_opts_t::GuardLow))
        type |= MemoryAllocationOptions::GuardLow;
    if (0 != (opts & mem_req_opts_t::GuardHigh))
        type |= MemoryAllocationOptions::GuardHigh;

    MemoryFlags flags = MemoryFlags::Userland;

    if (0 != (opts & mem_req_opts_t::Writable))
        flags |= MemoryFlags::Writable;
    if (0 != (opts & mem_req_opts_t::Executable))
        flags |= MemoryFlags::Executable;

    Handle res = Vmm::AllocatePages(nullptr, size / PageSize, type, flags, addr);

    if unlikely(!res.IsOkayResult())
        return res;

    return Handle(HandleType::Page, (uint64_t)reinterpret_cast<uintptr_t>(addr), false);
}

handle_t Syscalls::MemoryRelease(uintptr_t addr, size_t size, mem_rel_opts_t opts)
{
    if unlikely(addr != 0 && (addr < Vmm::UserlandStart || addr >= Vmm::UserlandEnd))
        return HandleResult::ArgumentOutOfRange;

    if unlikely(addr % PageSize != 0 || size % PageSize != 0)
        return HandleResult::AlignmentFailure;

    uintptr_t const end = addr + size;

    if unlikely(end < addr || end > Vmm::UserlandEnd)
        return HandleResult::ArgumentOutOfRange;

    return HandleResult::UnsupportedOperation;
}
