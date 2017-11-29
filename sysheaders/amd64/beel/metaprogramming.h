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

#ifndef __ASSEMBLER__
#include <stdint.h>

/*****************
    Some Types
*****************/

typedef uint64_t paddr_inner_t;
typedef uint64_t psize_inner_t;
typedef uint64_t vaddr_inner_t;
typedef uint64_t vsize_inner_t;
typedef uint64_t page_size_inner_t;
#endif

#include <beel/metaprogramming.x86.h>

#ifndef __BEELZEBUB__SOURCE_GAS
/*****************
    Some Types
*****************/

#ifdef __BEELZEBUB__SOURCE_CXX
namespace Beelzebub
{
#endif

typedef uint64_t  creg_t; //  Control register.

#ifdef __BEELZEBUB__SOURCE_CXX
}   //  namespace Beelzebub
#endif
typedef  int64_t ssize_t;

typedef  __int128_t  int128_t;
typedef __uint128_t uint128_t;

#endif

/*******************************
    Miscellaneous Assistance
*******************************/

#ifndef __BEELZEBUB__SOURCE_GAS
    #define _GAS_DATA_POINTER " .quad "

    #undef EXTEND_POINTER
    #define EXTEND_POINTER(ptr) __extension__ ({    \
        (0 != ((ptr) & 0x0000800000000000UL))       \
            ? (ptr) |= 0xFFFF000000000000UL         \
            : (ptr);                                \
    })

    #undef GET_EXTENDED_POINTER
    #define GET_EXTENDED_POINTER(ptr) __extension__ ({                              \
        uintptr_t const LINEVAR(__tmp_ptr) = REINTERPRET_CAST(uintptr_t, (ptr));    \
        (0 != (LINEVAR(__tmp_ptr) & 0x0000800000000000UL))                          \
            ? LINEVAR(__tmp_ptr) | 0xFFFF000000000000UL                             \
            : LINEVAR( __tmp_ptr);                                                  \
    })

    #define IS_CANONICAL(ptr) ((uint16_t)((REINTERPRET_CAST(uintptr_t, (ptr)) >> 48) + 1) <= 1)

    #define GET_CURRENT_STACK_POINTER() __extension__ ({                                \
        uintptr_t LINEVAR(__tmp_stk_ptr);                                               \
        asm volatile ("movq %%rsp, %[res] \n\t" : [res]"=rm"(LINEVAR(__tmp_stk_ptr)));  \
        LINEVAR(__tmp_stk_ptr);                                                         \
    })
#endif

#ifdef __BEELZEBUB__SOURCE_CXX
namespace Beelzebub
{
    template<typename T>
    static __forceinline uintptr_t GetExtendedPointer(T const ptr)
    {
        return GET_EXTENDED_POINTER(ptr);
    }

    static __forceinline uintptr_t GetCurrentStackPointer()
    {
        return GET_CURRENT_STACK_POINTER();
    }
}
#endif

/***************************
    Structure Assistance
***************************/

#if defined(__ASSEMBLER__)
    #define SIZE_OF_size_t 8
    #define SIZE_OF_intptr_t 8
    #define SIZE_OF_uintptr_t 8
#endif

/****************
    Constants
****************/

#ifdef __BEELZEBUB__SOURCE_CXX
namespace Beelzebub
{
    static constexpr PageSize_t const PageSize { 0x1000 };
    static constexpr PageSize_t const LargePageSize { 0x200000 };
}
#elif !defined(__ASSEMBLER__)
#define __PAGE_SIZE         ((size_t)0x1000)
#define __LARGE_PAGE_SIZE   ((size_t)0x200000)
#else
#define __PAGE_SIZE         0x1000
#define __LARGE_PAGE_SIZE   0x200000
#endif
