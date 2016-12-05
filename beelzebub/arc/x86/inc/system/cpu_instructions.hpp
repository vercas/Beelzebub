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

#include <beel/metaprogramming.h>

namespace Beelzebub { namespace System
{
    /**
     *  Represents a processing unit of the system.
     */
    class CpuInstructions
    {
        /*  Constructor(s)  */

    protected:
        CpuInstructions() = default;

    public:
        CpuInstructions(CpuInstructions const &) = delete;
        CpuInstructions & operator =(CpuInstructions const &) = delete;

        /*  Control  */

        static bool const CanHalt = true;

        static __forceinline void Halt()
        {
            asm volatile ( "hlt \n\t" : : : "memory" );
        }

        static __forceinline void DoNothing()
        {
            asm volatile ( "pause \n\t" : : : "memory" );
        }

        /*  Caching and Paging  */

        static __forceinline void WriteBackAndInvalidateCache()
        {
            asm volatile ( "wbinvd \n\t" : : : "memory" );
        }

        static __forceinline void InvalidateTlb(void const * const addr)
        {
            struct _4096_bytes { uint8_t x[4096]; } const * const p
            = reinterpret_cast<_4096_bytes const *>(addr);

            asm volatile ( "invlpg %0 \n\t" : : "m"(*p) );
        }

        static __forceinline void FlushCache(void const * const addr)
        {
            struct _64_bytes { uint8_t x[64]; } const * const p
            = reinterpret_cast<_64_bytes const *>(reinterpret_cast<uintptr_t>(addr) & ~((uintptr_t)0x3F));

            asm volatile ( "clflush %0 \n\t" : : "m"(*p) );
        }

        /*  Profiling  */

#if   defined(__BEELZEBUB__ARCH_AMD64)
        static __forceinline uint64_t Rdtsc()
        {
            uint64_t low, high;

            asm volatile ( "rdtsc \n\t" : "=a"(low), "=d"(high) );

            return (high << 32) | low;
        }
#else
        static __forceinline uint64_t Rdtsc()
        {
            uint64_t res;

            asm volatile ( "rdtsc \n\t" : "=A"(res) );

            return res;
        }
#endif

        /*  Far memory ops  */

        static __forceinline uint8_t FsGet8(uintptr_t const off)
        {
            uint8_t ret;

            asm volatile ( "movb %%fs:%1, %0 \n\t"
                         : "=r"(ret)
                         : "m"(*(reinterpret_cast<uintptr_t *>(off))) );

            return ret;
        }
        static __forceinline uint8_t FsSet8(uintptr_t const off, uint8_t const val)
        {
            asm volatile ( "movb %0, %%fs:%1 \n\t"
                         :
                         : "r"(val), "m"(*(reinterpret_cast<uintptr_t *>(off)))
                         : "memory" );

            return val;
        }

        static __forceinline uint8_t GsGet8(uintptr_t const off)
        {
            uint8_t ret;

            asm volatile ( "movb %%gs:%1, %0 \n\t"
                         : "=r"(ret)
                         : "m"(*(reinterpret_cast<uintptr_t *>(off))) );

            return ret;
        }
        static __forceinline uint8_t GsSet8(uintptr_t const off, uint8_t const val)
        {
            asm volatile ( "movb %0, %%gs:%1 \n\t"
                         :
                         : "r"(val), "m"(*(reinterpret_cast<uintptr_t *>(off)))
                         : "memory" );

            return val;
        }

        static __forceinline uint16_t FsGet16(uintptr_t const off)
        {
            uint16_t ret;

            asm volatile ( "movw %%fs:%1, %0 \n\t"
                         : "=r"(ret)
                         : "m"(*(reinterpret_cast<uintptr_t *>(off))) );

            return ret;
        }
        static __forceinline uint16_t FsSet16(uintptr_t const off, uint16_t const val)
        {
            asm volatile ( "movw %0, %%fs:%1 \n\t"
                         :
                         : "r"(val), "m"(*(reinterpret_cast<uintptr_t *>(off)))
                         : "memory" );

            return val;
        }

        static __forceinline uint16_t GsGet16(uintptr_t const off)
        {
            uint16_t ret;

            asm volatile ( "movw %%gs:%1, %0 \n\t"
                         : "=r"(ret)
                         : "m"(*(reinterpret_cast<uintptr_t *>(off))) );

            return ret;
        }
        static __forceinline uint16_t GsSet16(uintptr_t const off, uint16_t const val)
        {
            asm volatile ( "movw %0, %%gs:%1 \n\t"
                         :
                         : "r"(val), "m"(*(reinterpret_cast<uintptr_t *>(off)))
                         : "memory" );

            return val;
        }

        static __forceinline uint32_t FsGet32(uintptr_t const off)
        {
            uint32_t ret;

            asm volatile ( "movl %%fs:%1, %0 \n\t"
                         : "=r"(ret)
                         : "m"(*(reinterpret_cast<uintptr_t *>(off))) );

            return ret;
        }
        static __forceinline uint32_t FsSet32(uintptr_t const off, uint32_t const val)
        {
            asm volatile ( "movl %0, %%fs:%1 \n\t"
                         :
                         : "r"(val), "m"(*(reinterpret_cast<uintptr_t *>(off)))
                         : "memory" );

            return val;
        }

        static __forceinline uint32_t GsGet32(uintptr_t const off)
        {
            uint32_t ret;

            asm volatile ( "movl %%gs:%1, %0 \n\t"
                         : "=r"(ret)
                         : "m"(*(reinterpret_cast<uintptr_t *>(off))) );

            return ret;
        }
        static __forceinline uint32_t GsSet32(uintptr_t const off, uint32_t const val)
        {
            asm volatile ( "movl %0, %%gs:%1 \n\t"
                         :
                         : "r"(val), "m"(*(reinterpret_cast<uintptr_t *>(off)))
                         : "memory" );

            return val;
        }

#if   defined(__BEELZEBUB__ARCH_AMD64)
        static __forceinline uint64_t FsGet64(uintptr_t const off)
        {
            uint64_t ret;

            asm volatile ( "movq %%fs:%1, %0 \n\t"
                         : "=r"(ret)
                         : "m"(*(reinterpret_cast<uintptr_t *>(off))) );

            return ret;
        }
        static __forceinline uint64_t FsSet64(uintptr_t const off, uint64_t const val)
        {
            asm volatile ( "movq %0, %%fs:%1 \n\t"
                         :
                         : "r"(val), "m"(*(reinterpret_cast<uintptr_t *>(off)))
                         : "memory" );

            return val;
        }

        static __forceinline uint64_t GsGet64(uintptr_t const off)
        {
            uint64_t ret;

            asm volatile ( "movq %%gs:%1, %0 \n\t"
                         : "=r"(ret)
                         : "m"(*(reinterpret_cast<uintptr_t *>(off))) );

            return ret;
        }
        static __forceinline uint64_t GsSet64(uintptr_t const off, uint64_t const val)
        {
            asm volatile ( "movq %0, %%gs:%1 \n\t"
                         :
                         : "r"(val), "m"(*(reinterpret_cast<uintptr_t *>(off)))
                         : "memory" );

            return val;
        }
        static __forceinline uint64_t & GsXchg64(uintptr_t const off, uint64_t & val)
        {
            asm volatile ( "xchgq %0, %%gs:%1 \n\t"
                         :
                         : "r"(val), "m"(*(reinterpret_cast<uintptr_t *>(off)))
                         : "memory" );

            return val;
        }

        static __forceinline uintptr_t FsGetPointer(uintptr_t const off                     ) { return (uintptr_t)FsGet64(off     ); }
        static __forceinline uintptr_t FsSetPointer(uintptr_t const off, uintptr_t const val) { return (uintptr_t)FsSet64(off, val); }
        static __forceinline uintptr_t GsGetPointer(uintptr_t const off                     ) { return (uintptr_t)GsGet64(off     ); }
        static __forceinline uintptr_t GsSetPointer(uintptr_t const off, uintptr_t const val) { return (uintptr_t)GsSet64(off, val); }
        static __forceinline    size_t FsGetSize   (uintptr_t const off                     ) { return (   size_t)FsGet64(off     ); }
        static __forceinline    size_t FsSetSize   (uintptr_t const off,    size_t const val) { return (   size_t)FsSet64(off, val); }
        static __forceinline    size_t GsGetSize   (uintptr_t const off                     ) { return (   size_t)GsGet64(off     ); }
        static __forceinline    size_t GsSetSize   (uintptr_t const off,    size_t const val) { return (   size_t)GsSet64(off, val); }

#else
        static __forceinline uint64_t FsGet64(uintptr_t const off)
        {
            uint32_t low;
            uint32_t high;

            asm volatile ( "movl %%fs:%2, %0 \n\t"
                           "movl %%fs:%3, %1 \n\t"
                         : "=r"(low), "=r"(high)
                         : "m"(*(reinterpret_cast<uintptr_t *>(off    )))
                         , "m"(*(reinterpret_cast<uintptr_t *>(off + 4))) );

            return ((uint64_t)high << 32) | (uint64_t)low;
        }
        static __forceinline uint64_t FsSet64(uintptr_t const off, uint64_t const val)
        {
            asm volatile ( "movl %0, %%fs:%2 \n\t"
                           "movl %1, %%fs:%3 \n\t"
                         : 
                         : "r"((uint32_t)val)
                         , "r"((uint32_t)(val >> 32))
                         , "m"(*(reinterpret_cast<uintptr_t *>(off    )))
                         , "m"(*(reinterpret_cast<uintptr_t *>(off + 4)))
                         : "memory" );

            return val;
        }

        static __forceinline uint64_t GsGet64(uintptr_t const off)
        {
            uint32_t low;
            uint32_t high;

            asm volatile ( "movl %%gs:%2, %0 \n\t"
                           "movl %%gs:%3, %1 \n\t"
                         : "=r"(low), "=r"(high)
                         : "m"(*(reinterpret_cast<uintptr_t *>(off    )))
                         , "m"(*(reinterpret_cast<uintptr_t *>(off + 4))) );

            return ((uint64_t)high << 32) | (uint64_t)low;
        }
        static __forceinline uint64_t GsSet64(uintptr_t const off, uint64_t const val)
        {
            asm volatile ( "movl %0, %%gs:%2 \n\t"
                           "movl %1, %%gs:%3 \n\t"
                         : 
                         : "r"((uint32_t)val)
                         , "r"((uint32_t)(val >> 32))
                         , "m"(*(reinterpret_cast<uintptr_t *>(off    )))
                         , "m"(*(reinterpret_cast<uintptr_t *>(off + 4)))
                         : "memory" );

            return val;
        }

        static __forceinline uintptr_t FsGetPointer(uintptr_t const off                     ) { return (uintptr_t)FsGet32(off     ); }
        static __forceinline uintptr_t FsSetPointer(uintptr_t const off, uintptr_t const val) { return (uintptr_t)FsSet32(off, val); }
        static __forceinline uintptr_t GsGetPointer(uintptr_t const off                     ) { return (uintptr_t)GsGet32(off     ); }
        static __forceinline uintptr_t GsSetPointer(uintptr_t const off, uintptr_t const val) { return (uintptr_t)GsSet32(off, val); }
        static __forceinline    size_t FsGetSize   (uintptr_t const off                     ) { return (   size_t)FsGet32(off     ); }
        static __forceinline    size_t FsSetSize   (uintptr_t const off,    size_t const val) { return (   size_t)FsSet32(off, val); }
        static __forceinline    size_t GsGetSize   (uintptr_t const off                     ) { return (   size_t)GsGet32(off     ); }
        static __forceinline    size_t GsSetSize   (uintptr_t const off,    size_t const val) { return (   size_t)GsSet32(off, val); }

#endif

        static __forceinline uint32_t FarGet32(uint16_t const sel
                                             , uintptr_t const off)
        {
            uint32_t ret;

            asm volatile ( "push %%fs          \n\t"
                           "mov  %1, %%fs      \n\t"
                           "mov  %%fs:%2, %0 \n\t"
                           "pop  %%fs          \n\t"
                           : "=r"(ret)
                           : "g"(sel), "m"(*(reinterpret_cast<uintptr_t *>(off))) );

            return ret;
        }

        /*  Interrupts  */

        static __forceinline void Ltr(uint16_t const seg)
        {
            asm volatile ( "ltr %0 \n\t" : : "rm"(seg) );
        }

        /*  Miscellaneous  */

        static __forceinline void Clts()
        {
            asm volatile ( "clts \n\t" : : : "memory" );
        }
    };
}}
