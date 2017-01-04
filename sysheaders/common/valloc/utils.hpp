/*
    Copyright (c) 2017 Alexandru-Mihai Maftei. All rights reserved.


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

#include <stdint.h>
#include <stddef.h>

#ifdef __BEELZEBUB
    #define VALLOC_CACHE_LINE_POW2      (5)
    #define VALLOC_CACHE_LINE_SIZE      ((size_t)64)
    #define VALLOC_PAGE_SIZE            ((size_t)0x1000)

    #ifdef __BEELZEBUB__ARCH_AMD64
        #define VALLOC_POINTER_48BIT
        #define VALLOC_LARGE_PAGE_SIZE  ((size_t)0x200000)
    #elif defined(__BEELZEBUB__ARCH_IA32)
        #define VALLOC_LARGE_PAGE_SIZE  ((size_t)0x400000)
    #else
        #error "Unknown Beelzebub architecture."
    #endif

    #define VALLOC_STD_MUTEX

    #define VF_PTR "%Xp"
    #define VF_STR "%s"
#else
    #error "Please define parameters for your platform."
#endif

#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(__amd64__)
    #define VALLOC_PLAT_GCC_AMD64
    #define VALLOC_PLAT_AMD64
    #define VALLOC_PLAT_X86
    #define VALLOC_PLAT_GCC
#elif defined(_M_X64) || defined(_M_AMD64)
    #define VALLOC_PLAT_MSVC_AMD64
    #define VALLOC_PLAT_AMD64
    #define VALLOC_PLAT_X86
    #define VALLOC_PLAT_MSVC
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
    #define VALLOC_PLAT_GCC_IA32
    #define VALLOC_PLAT_IA32
    #define VALLOC_PLAT_X86
    #define VALLOC_PLAT_GCC
#elif defined(_M_IX86)
    #define VALLOC_PLAT_MSVC_IA32
    #define VALLOC_PLAT_IA32
    #define VALLOC_PLAT_X86
    #define VALLOC_PLAT_MSVC
#else
    #error "Unknown platform!"
#endif

#ifdef __GNUC__
    #define VALLOC_NORETURN   __attribute__((__noreturn__))
    #define VALLOC_PACKED     __attribute__((__packed__))
    #define VALLOC_ALIGNED(n) __attribute__((__aligned__(n)))
    #define VALLOC_CAN_ALIGN

    #define VALLOC_ANONYMOUS  __extension__

    #define VALLOC_LIKELY(expr)       (__builtin_expect(!!(expr), 1))
    #define VALLOC_UNLIKELY(expr)     (__builtin_expect((expr), 0))
#else
    #define VALLOC_NORETURN  
    #define VALLOC_PACKED  
    #define VALLOC_ALIGNED(n)  

    #define VALLOC_ANONYMOUS  
    
    #define VALLOC_LIKELY(expr)       (expr)
    #define VALLOC_UNLIKELY(expr)     (expr)
#endif

#ifdef VALLOC_SOURCE

#define VALLOC_ENUMOPS_LITE(T, U)                                                 \
    inline bool operator == (U   a, T b) { return               a  == (U )(b);  } \
    inline bool operator != (U   a, T b) { return               a  != (U )(b);  } \
    inline bool operator == (T   a, U b) { return         (U  )(a) ==      b ;  } \
    inline bool operator != (T   a, U b) { return         (U  )(a) !=      b ;  }

#define VALLOC_ENUMOPS_FULL(T, U)                                                 \
    inline  T   operator ~  (T   a     ) { return (T  )(~((U  )(a))          ); } \
    inline  T   operator |  (T   a, T b) { return (T  )(  (U  )(a) |  (U )(b)); } \
    inline  T   operator &  (T   a, T b) { return (T  )(  (U  )(a) &  (U )(b)); } \
    inline  T   operator &  (T   a, U b) { return (T  )(  (U  )(a) &       b ); } \
    inline  T   operator ^  (T   a, T b) { return (T  )(  (U  )(a) ^  (U )(b)); } \
    inline  T & operator |= (T & a, T b) { return (T &)(  (U &)(a) |= (U )(b)); } \
    inline  T & operator &= (T & a, T b) { return (T &)(  (U &)(a) &= (U )(b)); } \
    inline  T & operator ^= (T & a, T b) { return (T &)(  (U &)(a) ^= (U )(b)); } \
    VALLOC_ENUMOPS_LITE(T, U)

namespace Valloc
{
    template<typename T>
    static inline constexpr T * PointerAdd(T * const ptr, size_t const off)
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(ptr) + off);
    }

    template<typename T>
    static inline constexpr T * PointerSub(T * const ptr, size_t const off)
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(ptr) - off);
    }

    template<typename TNum1, typename TNum2>
    static inline constexpr auto RoundUp(const TNum1 value, const TNum2 step)
        -> decltype(((value + step - 1) / step) * step)
    {
        return ((value + step - 1) / step) * step;
    }

    typedef void (* PrintFunction)(char const *, ...);
}

#define VALLOC_ABORT() Platform::Abort(__FILE__, __LINE__, nullptr, nullptr)

#define VALLOC_ABORT_MSG(...) Platform::Abort(__FILE__, __LINE__, nullptr, __VA_ARGS__)

#define VALLOC_ASSERT(cond) [=](){ \
    if (VALLOC_UNLIKELY(!(cond))) Platform::Abort(__FILE__, __LINE__, #cond, "Assertion failure."); \
}()

#define VALLOC_ASSERT_MSG(cond, ...) [=](){ \
    if (VALLOC_UNLIKELY(!(cond))) Platform::Abort(__FILE__, __LINE__, #cond, __VA_ARGS__); \
}()

#endif
