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

/** 
 *  Yeah, there are C++-specific declarations too.
 *  The C and common ones are defined in 'string.c'.
 *  The C++-specific ones are defined in 'string.cpp'.
 */

#pragma once

#include <beel/metaprogramming.h>

__shared_inline bool memeq(void const * src1, void const * src2, size_t len);
__shared_inline comp_t memcmp(void const * src1, void const * src2, size_t len);

__shared_inline void * memchr(void const * src, int val, size_t len);

__shared_inline void * memcpy(void * dst, void const * src, size_t len);
__shared_inline void * memmove(void * dst, void const * src, size_t len);
__shared_inline void * mempcpy(void * dst, void const * src, size_t len);
__shared_inline void * mempmove(void * dst, void const * src, size_t len);

__shared_inline void * memset(void * dst, int const val, size_t len);
__shared_inline void * mempset(void * dst, int const val, size_t len);
__shared_inline void * memset16(void * dst, int const val, size_t cnt);
__shared_inline void * mempset16(void * dst, int const val, size_t cnt);
__shared_inline void * memset32(void * dst, long const val, size_t cnt);
__shared_inline void * mempset32(void * dst, long const val, size_t cnt);

__shared_inline size_t strlen(char const * str);
__shared_inline size_t strnlen(char const * str, size_t len);
__shared_inline size_t strnlenex(char const * str, size_t len, bool * reached);

__shared_inline char * strcat(char * dst, char const * src);
__shared_inline char * strncat(char * dst, char const * src, size_t len);

__shared_inline char * strcpy(char * dst, char const * src);
__shared_inline char * strncpy(char * dst, char const * src, size_t len);

__shared_inline char * strpbrk(char const * haystack, char const * needle);
__shared_inline char * strnpbrk(char const * haystack, char const * needle, size_t len);

__shared_inline comp_t strcmp(char const * src1, char const * src2);
__shared_inline comp_t strncmp(char const * src1, char const * src2, size_t len);

__shared_inline comp_t strcasecmp(char const * src1, char const * src2);
__shared_inline comp_t strcasencmp(char const * src1, char const * src2, size_t len);

__shared_inline char * strchr(char const * haystack, int needle);
__shared_inline char * strstr(char const * haystack, char const * needle);

__shared_inline char const * strstrex(char const * haystack, char const * needle, char const * seps);
__shared_inline char const * strcasestrex(char const * haystack, char const * needle, char const * seps);

__shared char const * strerrorc(int errnum);
__shared char * strerror(int errnum);

#ifdef _GNU_SOURCE
__shared char * strerror_r(int errnum, char * buf, size_t buflen);
            /* GNU-specific */
#else
__shared int strerror_r(int errnum, char * buf, size_t buflen);
            /* XSI-compliant */
#endif

#ifdef __BEELZEBUB__SOURCE_CXX
namespace Beelzebub
{

#define OPS1A(Ta, Tb, Tl) \
__forceinline bool memeq(const Ta src1, const Tb src2, Tl len) \
{ return ::memeq((void const *)src1, (void const *)src2, (size_t)len); } \
__forceinline comp_t memcmp(const Ta src1, const Tb src2, Tl len) \
{ return ::memcmp((void const *)src1, (void const *)src2, (size_t)len); } \
__forceinline void * memcpy(Ta dst, const Tb src, Tl len) \
{ return ::memcpy((void *)dst, (void const *)src, (size_t)len); } \
__forceinline void * memmove(Ta dst, const Tb src, Tl len) \
{ return ::memmove((void *)dst, (void const *)src, (size_t)len); } \
__forceinline void * mempcpy(Ta dst, const Tb src, Tl len) \
{ return ::mempcpy((void *)dst, (void const *)src, (size_t)len); } \
__forceinline void * mempmove(Ta dst, const Tb src, Tl len) \
{ return ::mempmove((void *)dst, (void const *)src, (size_t)len); }

#define OPS1B(Ta, Tl) \
__forceinline void * memchr(const Ta src, int val, Tl len) \
{ return ::memchr((void const *)src, val, (size_t)len); } \
__forceinline void * memset(Ta dst, int const val, Tl len) \
{ return ::memset((void *)dst, val, (size_t)len); }

OPS1A(vaddr_t, vaddr_t, size_t) OPS1B(vaddr_t, size_t)
OPS1A(void *, vaddr_t, size_t) OPS1A(vaddr_t, void *, size_t)

OPS1A(vaddr_t, vaddr_t, vsize_t) OPS1B(vaddr_t, vsize_t)
OPS1A(void *, vaddr_t, vsize_t) OPS1A(vaddr_t, void *, vsize_t)
OPS1B(void *, vsize_t)

OPS1A(vaddr_t, vaddr_t, PageSize_t) OPS1B(vaddr_t, PageSize_t)
OPS1A(void *, vaddr_t, PageSize_t) OPS1A(vaddr_t, void *, PageSize_t)
OPS1B(void *, PageSize_t)
#undef OPS1A
#undef OPS1B
}
#endif
