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

#include <valloc/utils.hpp>

namespace Valloc
{
    /**
     *  <summary>Represents an abstraction of the platform.</summary>
     */
    class Platform
    {
    public:
        /*  Statics  */

#ifdef VALLOC_SIZES_NONCONST
        static size_t CacheLineSize;
        static size_t PageSize;
        static size_t LargePageSize;
#else
        static constexpr size_t const CacheLinePow2 = VALLOC_CACHE_LINE_POW2;
        static constexpr size_t const CacheLineSize = VALLOC_CACHE_LINE_SIZE;
        static constexpr size_t const PageSize = VALLOC_PAGE_SIZE;
        static constexpr size_t const LargePageSize = VALLOC_LARGE_PAGE_SIZE;
#endif

    protected:
        /*  Constructor(s)  */

        Platform() = default;

    public:
        Platform(Platform const &) = delete;
        Platform & operator =(Platform const &) = delete;

        /*  Memory  */

        static void AllocateMemory(void * & addr, size_t & size);
        static void FreeMemory(void * addr, size_t size);

        /*  Debug  */

        static void ErrorMessage(char const * fmt, ...);
        static VALLOC_NORETURN void Abort(char const * file, size_t line);

        /*  Atomics  */

        template<typename T>
        static inline T Swap(T * const val, T const des)
        {
#ifdef VALLOC_PLAT_GCC
            return __atomic_exchange_n(val, des, __ATOMIC_SEQ_CST);
#else
    #error "TODO!"
#endif
        }

        template<typename T>
        static inline bool CAS(T * const val, T & exp, T const des)
        {
#ifdef VALLOC_PLAT_GCC
            return __atomic_compare_exchange_n(val, &exp, des, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE);
#else
    #error "TODO!"
#endif
        }

        template<typename T0, typename T1, typename T2>
        static inline bool CAS2(T0 * const val, T1 & exp1, T2 & exp2, T1 const des1, T2 const des2)
        {
#ifdef VALLOC_PLAT_GCC
    #ifdef VALLOC_PLAT_X86
        #ifdef VALLOC_PLAT_AMD64
            #define TWOPTR_SIZE "16"
            static_assert(sizeof(T0) == 16, "Size mismatch.");
        #elif defined(VALLOC_PLAT_IA32)
            #define TWOPTR_SIZE "8"
            static_assert(sizeof(T0) == 8, "Size mismatch.");
        #else
            #error "Whut?"
        #endif

        #ifdef __GCC_ASM_FLAG_OUTPUTS__
            int res;
        #else
            bool res;
        #endif

            asm volatile( "lock cmpxchg" TWOPTR_SIZE "b %[val] \n\t"
        #ifndef __GCC_ASM_FLAG_OUTPUTS__
                          "setzb %[res]                      \n\t"
        #endif
                : [val]"+m"(*val), "+a"(exp1), "+d"(exp2)
        #ifdef __GCC_ASM_FLAG_OUTPUTS__
                , "=@ccz"(res) /* zero flag  */
        #else
                , [res]"=qm"(res)
        #endif
                : "b"(des1), "c"(des2)
                : "cc" );

        #ifdef __GCC_ASM_FLAG_OUTPUTS__
            return (bool)res;
        #else
            return res;
        #endif

        #undef TWOPTR_SIZE
    #endif
#else
    #error "TODO!"
#endif
        }
    };
}
