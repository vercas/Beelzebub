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

//////////////////////////////////////////////
// All syscalls are included at the bottom! //
//////////////////////////////////////////////

#pragma once

#include <beel/syscalls.h>
#include <beel/structs.kernel.common.h>

#ifdef __BEELZEBUB__SOURCE_CXX
    #define POST_MESSAGE(n) PostMessage
#else
    #define POST_MESSAGE(n) MCATS(PostMessage, n)
#endif

#ifdef __BEELZEBUB__SOURCE_CXX
#define SYSCALL_POST_MESSAGE (SyscallSelection::PostMessage)
#define SYSCALL_RECEIVE_MESSAGE (SyscallSelection::ReceiveMessage)
namespace Beelzebub
{
#endif
    __forceinline Handle POST_MESSAGE(HHHHH)(Handle dst, Handle src
        , uint64_t flags, Handle h1, Handle h2, Handle h3, Handle h4, Handle h5)
    {
        Handle res;

        PUT_IN_REGISTER(r10, h1);
        PUT_IN_REGISTER(r8 , h2);
        PUT_IN_REGISTER(r9 , h3);
        PUT_IN_REGISTER(r12, h4);
        PUT_IN_REGISTER(r13, h5);

        asm volatile ( "syscall \n\t"
                     : "=a"(res)
                     , "+D"(dst), "+S"(src), "+d"(flags)
                     , "+r"(r10), "+r"(r8), "+r"(r9), "+r"(r12), "+r"(r13)
                     : "a"(SYSCALL_POST_MESSAGE)
                     : "rcx", "r11", "memory");

        return res;
    }

    __forceinline Handle ReceiveMessage(Message * const msg)
    {
        Handle res;

        REGISTER_VARIABLE(r10);
        REGISTER_VARIABLE(r8 );
        REGISTER_VARIABLE(r9 );
        REGISTER_VARIABLE(r12);
        REGISTER_VARIABLE(r13);

        asm volatile ( "syscall \n\t"
                     : "=a"(res)
                     , "=D"(msg->Destination), "=S"(msg->Source), "=d"(msg->Flags)
                     , "=r"(r10), "=r"(r8), "=r"(r9), "=r"(r12), "=r"(r13)
                     : "a"(SYSCALL_RECEIVE_MESSAGE)
                     : "rcx", "r11", "memory");

        msg->D[0] = r10;
        msg->D[1] = r8;
        msg->D[2] = r9;
        msg->D[3] = r12;
        msg->D[4] = r13;

        return res;
    }
#ifdef __BEELZEBUB__SOURCE_CXX
}
#endif
