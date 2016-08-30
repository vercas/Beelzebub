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

#include <syscalls.kernel.hpp>

#include <syscalls/memory.h>

#include <terminals/base.hpp>

#include <memory/vmm.hpp>
#include <system/cpu.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Syscalls;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

Handle Beelzebub::SyscallCommon(SyscallSelection const selector, void * arg1
                              , uintptr_t arg2, uintptr_t arg3
                              , uintptr_t arg4, uintptr_t arg5)
{
    switch (selector)
    {
    case SyscallSelection::DebugPrint:
        {
            if (arg2 > (1 << 16))
                return HandleResult::ArgumentOutOfRange;

            Handle res = Vmm::CheckMemoryRegion(nullptr
                , reinterpret_cast<uintptr_t>(arg1), arg2
                , MemoryCheckType::Userland | MemoryCheckType::Readable);

            assert(res.IsOkayResult()
                , "Debug print string failure: %H%n"
                  "%Xp %up: %s%n"
                , res, arg1, arg2, arg1);

            if unlikely(!res.IsOkayResult())
                return res;

            if (arg3 == 0)
                return Debug::DebugTerminal->Write(reinterpret_cast<char *>(arg1), arg2).Result;
            else if (0 == (arg3 & 0x3))
            {
                //  The 3rd argument must be in the userland memory region and 4-byte aligned.

                res = Vmm::CheckMemoryRegion(nullptr, arg3, 4
                    , MemoryCheckType::Userland | MemoryCheckType::Writable);

                if unlikely(!res.IsOkayResult())
                    return res;

                uint32_t * const sizePtr = reinterpret_cast<uint32_t *>(arg3);
                
                TerminalWriteResult twRes = Debug::DebugTerminal->Write(reinterpret_cast<char *>(arg1), arg2);

                *sizePtr = twRes.Size;
                return twRes.Result;
            }
            else
                return HandleResult::ArgumentOutOfRange;
        }

    case SyscallSelection::MemoryRequest:
        return MemoryRequest(reinterpret_cast<uintptr_t>(arg1), (size_t)arg2, (mem_req_opts_t)arg3);

    case SyscallSelection::MemoryRelease:
        return MemoryRelease(reinterpret_cast<uintptr_t>(arg1), (size_t)arg2, (mem_rel_opts_t)arg3);

    case SyscallSelection::MemoryCopy:
        return MemoryCopy(reinterpret_cast<uintptr_t>(arg1), arg2, (size_t)arg3);

    default:
        return HandleResult::SyscallSelectionInvalid;
    }

    return HandleResult::Okay;
}
