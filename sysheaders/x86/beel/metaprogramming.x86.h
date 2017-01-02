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

#include <beel/metaprogramming.common.h>

/*****************
    Attributes
*****************/

#if defined(__GNUC__) && !defined(__clang__)
    #define __bland __attribute__((__target__("no-aes,no-mmx,no-pclmul,no-sse,"     \
                                              "no-sse2,no-sse3,no-sse4,no-sse4a,"   \
                                              "no-fma4,no-lwp,no-ssse3,"            \
                                              "no-fancy-math-387,no-ieee-fp,"       \
                                              "no-recip")))
    #define __fancy __attribute__((__target__("aes,mmx,pclmul,sse,"         \
                                              "sse2,sse3,sse4,sse4a,"       \
                                              "fma4,lwp,ssse3,"             \
                                              "fancy-math-387,ieee-fp,"     \
                                              "recip")))
    #define __sse2 __attribute__((__target__("mmx,sse,sse2,fancy-math-387," \
                                             "ieee-fp")))
    #define __fpu __attribute__((__target__("fancy-math-387,ieee-fp")))

    #if   defined(__BEELZEBUB__ARCH_AMD64)
        #define __min_float __sse2
    #else
        #define __min_float __fpu
    #endif
#else
    #define __bland  
    #define __fancy  
    #define __sse2  
    #define __fpu  
    #define __min_float  
#endif

#if !defined(__ASSEMBLER__)
/*****************
    Some Types
*****************/

typedef int comp_t; //  Result of comparison functions.

#endif

/****************
    Constants
****************/

#ifdef __BEELZEBUB__SOURCE_CXX
namespace Beelzebub
{
    static constexpr size_t const PageSize = 0x1000;
}
#elif !defined(__ASSEMBLER__)
#define __PAGE_SIZE         ((size_t)0x1000)
#else
#define __PAGE_SIZE         0x1000
#endif
