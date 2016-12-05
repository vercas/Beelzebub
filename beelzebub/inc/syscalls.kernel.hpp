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

#pragma once

#include <beel/syscalls.h>

namespace Beelzebub
{
    typedef Handle (* SyscallFunction)(void * arg0, void * arg1, void * arg2
                                     , void * arg3, void * arg4, void * arg5
                                     , void * const stackptr, SyscallSelection const selector);

    struct SyscallSlot
    {
        /*  Constructors  */

        inline SyscallSlot() : Value() { }

        template<typename TFunc>
        inline SyscallSlot(TFunc const val)
            : Value(reinterpret_cast<void *>(val))
        { }

        /*  Operations  */

        inline SyscallFunction GetFunction() const
        {
            return reinterpret_cast<SyscallFunction>(this->Value);
        }

        inline bool IsImplemented() const
        {
            return this->Value != nullptr;
        }

        /*  Fields  */

        void * Value;
    };

    extern SyscallSlot DefaultSystemCalls[(size_t)SyscallSelection::COUNT];

    __extern __hot __fastcall_ia32 Handle SyscallCommon(void * arg0, void * arg1, void * arg2
                                                      , void * arg3, void * arg4, void * arg5
                                                      , void * const stackptr, SyscallSelection const selector);
    //  A selector and 5 arguments, platform-independent.
    //  Argument 1 (2nd function argument) is the one used to point to extra
    //  data, if 5 syscall arguments are not enough.
    //  The IA-32 architecture will use the fastcall convention here, so the
    //  userland can pass on at least one argument as register and the rest on
    //  the stack.
}
