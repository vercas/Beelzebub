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
#include <system/cpu.hpp>
#include <math.h>
#include <string.h>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Syscalls;

static constexpr size_t const ChunkSize = 4 * 1 << 20;  //  4 MiB.

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

    Handle res = Vmm::AllocatePages(nullptr, size / PageSize, type, flags, MemoryContent::Generic, addr);

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

handle_t Syscalls::MemoryCopy(uintptr_t dst, uintptr_t src, size_t len)
{
    if unlikely(dst == src || len == 0)
        return HandleResult::Okay;

    if unlikely(dst + len < dst || src + len < src
        || dst < Vmm::UserlandStart || (dst + len) >= Vmm::UserlandEnd)
        return HandleResult::ArgumentOutOfRange;
    //  Overflow and boundaries check. Source need not be in userland half.

    Handle res = Vmm::CheckMemoryRegion(nullptr, src, len
        , MemoryCheckType::Userland | MemoryCheckType::Readable);
    //  Source has to be accessible by userland, though.

    assert_or(res.IsOkayResult()
        , "Memory copy syscall source check failure: %H%n"
          "dst = %Xp; src = %Xp; len = %up%n"
        , res, dst, src, len)
    {
        return res;
    }

    res = Vmm::CheckMemoryRegion(nullptr, dst, len
        , MemoryCheckType::Userland | MemoryCheckType::Readable
        | MemoryCheckType::Private);
    //  Destination does *NOT* need to be writable! This syscall exists specifically
    //  for userland to be able to modify its own read-only pages.

    assert_or(res.IsOkayResult()
        , "Memory copy syscall destination check failure: %H%n"
          "dst = %Xp; src = %Xp; len = %up%n"
        , res, dst, src, len)
    {
        return res;
    }

    //  So everything *appears* to be okay. This will not be true for long.
    //  TODO: Lock onto the regions or something like that. Or use kernel's exceptions
    //  to catch a page fault if either the source or destination changed.

    //  Anyhow, this will perform the copy in 4-MiB chunks, between which the syscall
    //  may actually be interrupted.

    for (size_t chunk = 0; chunk < len; chunk += ChunkSize, dst += ChunkSize, src += ChunkSize)
    {
        size_t curChunk = Minimum(ChunkSize, len - chunk);

        withInterrupts (false)
        withWriteProtect (false)
            memmove(reinterpret_cast<void *>(dst), reinterpret_cast<void const *>(src), curChunk);
        //  TODO: Exception handling, maybe?
    }

    return HandleResult::Okay;
}

handle_t Syscalls::MemoryFill(uintptr_t dst, uint8_t val, size_t len)
{
    if unlikely(len == 0)
        return HandleResult::Okay;

    if unlikely(dst + len < dst
        || dst < Vmm::UserlandStart || (dst + len) >= Vmm::UserlandEnd)
        return HandleResult::ArgumentOutOfRange;
    //  Overflow and boundaries check.

    Handle res = Vmm::CheckMemoryRegion(nullptr, dst, len
        , MemoryCheckType::Userland | MemoryCheckType::Readable
        | MemoryCheckType::Private);
    //  Destination does *NOT* need to be writable! This syscall exists specifically
    //  for userland to be able to modify its own read-only pages.

    assert_or(res.IsOkayResult()
        , "Memory fill syscall destination check failure: %H%n"
          "dst = %Xp; val = 0x%X1; len = %up%n"
        , res, dst, val, len)
    {
        return res;
    }

    //  TODO: Lock onto the regions or something like that. Or use kernel's exceptions
    //  to catch a page fault if destination changed.

    //  Anyhow, this will perform the filling in 4-MiB chunks, between which the syscall
    //  may actually be interrupted.

    for (size_t chunk = 0; chunk < len; chunk += ChunkSize, dst += ChunkSize)
    {
        size_t curChunk = Minimum(ChunkSize, len - chunk);

        withInterrupts (false)
        withWriteProtect (false)
            memset(reinterpret_cast<void *>(dst), val, curChunk);
        //  TODO: Exception handling, maybe?
    }

    return HandleResult::Okay;
}
