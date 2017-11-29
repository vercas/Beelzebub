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

#include <beel/terminals/base.hpp>

#include <memory/vmm.hpp>
#include <system/cpu.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

SyscallSlot Beelzebub::DefaultSystemCalls[(size_t)SyscallSelection::COUNT] = {0};

Handle Beelzebub::SyscallCommon(void * arg0, void * arg1, void * arg2
                              , void * arg3, void * arg4, void * arg5
                              , void * const stackptr, SyscallSelection const selector)
{
    if (selector >= SyscallSelection::COUNT)
        return HandleResult::SyscallSelectionInvalid;
    //  TODO: Ask kernel modules for their syscalls..?

    SyscallSlot & slot = DefaultSystemCalls[(size_t)selector];

    if unlikely(!slot.IsImplemented())
    {
        switch (selector)
        {
        case SyscallSelection::DebugPrint:
            {
                if (reinterpret_cast<size_t>(arg1) > (1 << 16))
                    return HandleResult::ArgumentOutOfRange;

                Handle res = Vmm::CheckMemoryRegion(nullptr
                    , vaddr_t(arg0)
                    , vsize_t(reinterpret_cast<uintptr_t>(arg1))
                    , MemoryCheckType::Userland | MemoryCheckType::Readable);

                assert_or(res.IsOkayResult()
                    , "Debug print string failure: %H%n"
                      "%Xp %up: %s%n"
                    , res, arg0, arg1, arg0)
                {
                    return res;
                }

                if (arg2 == nullptr)
                    return Debug::DebugTerminal->Write((char *)arg0, reinterpret_cast<size_t>(arg1)).Result;
                else if (0 == (reinterpret_cast<uintptr_t>(arg2) & 0x3))
                {
                    //  The 3rd argument must be in the userland memory region and 4-byte aligned.

                    res = Vmm::CheckMemoryRegion(nullptr
                        , vaddr_t(arg2), vsize_t(sizeof(uint32_t))
                        , MemoryCheckType::Userland | MemoryCheckType::Writable);

                    if unlikely(!res.IsOkayResult())
                        return res;

                    uint32_t * const sizePtr = reinterpret_cast<uint32_t *>(arg2);

                    TerminalWriteResult twRes = Debug::DebugTerminal->Write((char *)arg0, reinterpret_cast<size_t>(arg1));

                    *sizePtr = twRes.Size;
                    return twRes.Result;
                }
                else
                    return HandleResult::ArgumentOutOfRange;
            }

        default:
            return HandleResult::SyscallSelectionInvalid;
        }

        return HandleResult::Okay;
    }
    else
        return slot.GetFunction()(arg0, arg1, arg2, arg3, arg4, arg5, stackptr, selector);
}
