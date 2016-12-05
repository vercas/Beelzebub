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

#pragma once

#include <beel/handles.h>
#include <beel/syscall.selection.h>

namespace Beelzebub
{
    enum class SyscallSelection : unsigned
    {
        ENUM_SYSCALLSELECTION(ENUMINST_VAL)
    };

    ENUMOPS_LITE(SyscallSelection)

    __forceinline Handle PerformSyscall(SyscallSelection selection
        , void * arg0, void * arg1, void * arg2
        , void * arg3, void * arg4, void * arg5)
    {
        Handle res;

        register void * r10 asm("r10") = arg3;
        register void * r8  asm("r8" ) = arg4;
        register void * r9  asm("r9" ) = arg5;

        asm volatile ( "syscall \n\t"
                     : "=a"(res)
                     , "+D"(arg0), "+S"(arg1), "+d"(arg2)
                     , "+r"(r10), "+r"(r8), "+r"(r9)
                     : "a"(selection)
                     : "rcx", "r11", "memory");

        return res;
    }

    __forceinline Handle PerformSyscall(SyscallSelection selection
        , void * arg0, void * arg1, void * arg2
        , void * arg3, void * arg4)
    {
        Handle res;

        register void * r10 asm("r10") = arg3;
        register void * r8  asm("r8" ) = arg4;

        asm volatile ( "syscall \n\t"
                     : "=a"(res)
                     , "+D"(arg0), "+S"(arg1), "+d"(arg2)
                     , "+r"(r10), "+r"(r8)
                     : "a"(selection)
                     : "rcx", "r11", "memory");

        return res;
    }

    __forceinline Handle PerformSyscall(SyscallSelection selection
        , void * arg0, void * arg1, void * arg2
        , void * arg3)
    {
        Handle res;

        register void * r10 asm("r10") = arg3;

        asm volatile ( "syscall \n\t"
                     : "=a"(res)
                     , "+D"(arg0), "+S"(arg1), "+d"(arg2)
                     , "+r"(r10)
                     : "a"(selection)
                     : "rcx", "r11", "memory");

        return res;
    }

    __forceinline Handle PerformSyscall(SyscallSelection selection
        , void * arg0, void * arg1, void * arg2)
    {
        Handle res;

        asm volatile ( "syscall \n\t"
                     : "=a"(res)
                     , "+D"(arg0), "+S"(arg1), "+d"(arg2)
                     : "a"(selection)
                     : "rcx", "r11", "memory");

        return res;
    }

    __forceinline Handle PerformSyscall(SyscallSelection selection
        , void * arg0, void * arg1)
    {
        Handle res;

        asm volatile ( "syscall \n\t"
                     : "=a"(res)
                     , "+D"(arg0), "+S"(arg1)
                     : "a"(selection)
                     : "rcx", "r11", "memory");

        return res;
    }

    __forceinline Handle PerformSyscall(SyscallSelection selection
        , void * arg0)
    {
        Handle res;

        asm volatile ( "syscall \n\t"
                     : "=a"(res)
                     , "+D"(arg0)
                     : "a"(selection)
                     : "rcx", "r11", "memory");

        return res;
    }

    __forceinline Handle PerformSyscall(SyscallSelection selection)
    {
        Handle res;

        asm volatile ( "syscall \n\t"
                     : "=a"(res)
                     : "a"(selection)
                     : "rcx", "r11", "memory");

        return res;
    }
}

#undef ENUM_SYSCALLSELECTION
