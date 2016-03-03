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
#else
    #define __bland  
    #define __fancy  
#endif

//  All hail the preprocessor!

//  Some type definitions.

#if   defined(__BEELZEBUB__ARCH_AMD64)
typedef uint64_t paddr_t; //  Physical address.
typedef uint64_t vaddr_t; //  Virtual (linear) address.
typedef uint64_t psize_t; //  Physical size.
typedef uint64_t vsize_t; //  Virtual (linear) address.
typedef uint64_t pgind_t; //  Index of a memory page.
typedef uint64_t  creg_t; //  Control register.

typedef  int64_t ssize_t;

typedef void * int_cookie_t;

#elif defined(__BEELZEBUB__ARCH_IA32)

#if   defined(__BEELZEBUB__ARCH_IA32PAE)
typedef uint64_t paddr_t; //  Physical address.
typedef uint64_t psize_t; //  Physical size.
#else
typedef uint32_t paddr_t; //  Physical address.
typedef uint32_t psize_t; //  Physical size.
#endif

typedef uint32_t vaddr_t; //  Virtual (linear) address.
typedef uint32_t vsize_t; //  Virtual (linear) address.
typedef uint32_t pgind_t; //  Index of a memory page.
typedef uint32_t  creg_t; //  Control register.

typedef  int32_t ssize_t;

typedef void * int_cookie_t;

#define __fastcall_ia32 __attribute__((__fastcall__))

#endif

typedef uint16_t seg_t; //  Segment register.

#ifdef __GNUC__
typedef union
{
    void * ptr;
    vaddr_t val;
} vaddrptr_t __attribute__((transparent_union));
#endif

typedef             int     comp_t; //  Result of comparison functions.

typedef unsigned     char     byte; //  Uhm...
typedef   signed     char    sbyte; //  Yeah...

#if   !defined(__cplusplus) && defined(__BEELZEBUB_KERNEL)
typedef char bool;

#define true    ((bool)1)
#define TRUE    ((bool)1)
#define false   ((bool)0)
#define FALSE   ((bool)0)
#endif

/*  Constants  */

#if   defined(__cplusplus)
enum size_consts : size_t { PageSize = 0x1000, };
#define int_cookie_invalid nullptr
//constexpr size_t const PageSize = 0x1000;
//constexpr int_cookie_t const int_cookie_invalid = nullptr;
#else
#define PAGE_SIZE ((size_t)0x1000)
#define INT_COOKIE_INVALID (NULL)
#endif

//  Bit 3 is reserved as 0, so the cookie will never be 8. Moreover, bit 1 is
//  always 1, so really the flags will never be 8.
