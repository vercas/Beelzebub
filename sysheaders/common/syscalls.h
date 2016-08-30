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

#include <handles.h>

#define ENUM_SYSCALLSELECTION(ENUMINST) \
    /*  Will simply print a value on the debug terminal. */ \
    ENUMINST(DebugPrint   , SYSCALL_DEBUG_PRINT   ,   0, "Debug Print"   ) \
    /*  Requests a number of pages of memory from the OS. */ \
    ENUMINST(MemoryRequest, SYSCALL_MEMORY_REQUEST, 100, "Memory Request") \
    /*  Releases a number of pages of memory to the OS. */ \
    ENUMINST(MemoryRelease, SYSCALL_MEMORY_RELEASE, 101, "Memory Release") \
    /*  Copies a chunk of memory to the target address. */ \
    ENUMINST(MemoryCopy   , SYSCALL_MEMORY_COPY   , 102, "Memory Copy"   )

typedef
#ifdef __cplusplus
enum class SyscallSelection
#else
enum SYSCALLS
#endif
{
    ENUM_SYSCALLSELECTION(ENUMINST_VAL)
} syscall_selection_t;

#undef ENUM_SYSCALLSELECTION

#include <syscall.inc>
