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
#include <syscalls.h>

using namespace Beelzebub;

handle_t Beelzebub::MemoryRequest(uintptr_t addr, size_t size, mem_req_opts_t opts)
{
    if unlikely(addr % PageSize != 0 || size % PageSize != 0)
        return HandleResult::AlignmentFailure;
    //  The kernel will also perform this check.

    if unlikely((addr + size) < addr)
        return HandleResult::ArgumentOutOfRange;

    return PerformSyscall3(SyscallSelection::MemoryRequest
        , reinterpret_cast<void *>(addr), (uintptr_t)size, (uintptr_t)(int)opts);
}

handle_t Beelzebub::MemoryRelease(uintptr_t addr, size_t size, mem_rel_opts_t opts)
{
    if unlikely(addr % PageSize != 0 || size % PageSize != 0)
        return HandleResult::AlignmentFailure;
    //  Ditto.

    if unlikely((addr + size) < addr)
        return HandleResult::ArgumentOutOfRange;

    return PerformSyscall3(SyscallSelection::MemoryRelease
        , reinterpret_cast<void *>(addr), (uintptr_t)size, (uintptr_t)(int)opts);
}

handle_t Beelzebub::MemoryCopy(uintptr_t dst, uintptr_t src, size_t len)
{
    if unlikely(dst == src || len == 0)
        return HandleResult::Okay;

    //  TODO?: Attempt a normal `memmov` and catch an exception somehow?

    return PerformSyscall3(SyscallSelection::MemoryCopy
        , reinterpret_cast<void *>(dst), src, (uintptr_t)len);
}

handle_t Beelzebub::MemoryFill(uintptr_t dst, uint8_t val, size_t len)
{
    if unlikely(len == 0)
        return HandleResult::Okay;

    //  TODO?: Attempt a normal `memset` and catch an exception somehow?

    return PerformSyscall3(SyscallSelection::MemoryFill
        , reinterpret_cast<void *>(dst), (uintptr_t)val, (uintptr_t)len);
}
