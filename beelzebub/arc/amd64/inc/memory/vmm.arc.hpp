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
 *  The implementation of the virtual memory manager is architecture-specific.
 */

#pragma once

#include "memory/paging.hpp"
#include <beel/sync/smp.lock.hpp>
#include <beel/handles.h>

namespace Beelzebub { namespace Memory
{
    /**
     *  The architecture-specific aspects of the virtual memory manager.
     */
    class VmmArc
    {
    public:
        /*  Statics  */

        static constexpr uintptr_t const IsaDmaStart         = 0xFFFF800000000000ULL;
        static constexpr size_t    const IsaDmaLength        = 1ULL << 24;  //  16 MiB
        static constexpr uintptr_t const IsaDmaEnd           = IsaDmaStart + IsaDmaLength;

        static constexpr uintptr_t const KernelHeapStart     = IsaDmaEnd;
        static constexpr uintptr_t const KernelHeapEnd       = 0xFFFFFE0000000000ULL;
        static constexpr size_t    const KernelHeapLength    = KernelHeapEnd - KernelHeapStart;
        static constexpr size_t    const KernelHeapPageCount = KernelHeapLength >> 12;

        static bool Page1GB, NX, PCID;

        static __thread paddr_t LastAlienPml4;

        //  End of the lower half address range.
        static constexpr uintptr_t const LowerHalfEnd    = 0x0000800000000000ULL;
        //  Start of the higher half address range.
        static constexpr uintptr_t const HigherHalfStart = 0xFFFF800000000000ULL;

        //  Right below the kernel.
        static constexpr uint16_t const LocalFractalIndex = 510;
        //  Right below the local PML4.
        static constexpr uint16_t const AlienFractalIndex = 509;

        //  Start of the active PML4 tables.
        static constexpr uintptr_t const LocalPml1Base = 0xFFFF000000000000ULL + ((uint64_t)LocalFractalIndex << 39);
        //  Start of the active PML3 tables (PDPTs).
        static constexpr uintptr_t const LocalPml2Base = LocalPml1Base + ((uint64_t)LocalFractalIndex << 30);
        //  Start of the active PML2 tables (PDs).
        static constexpr uintptr_t const LocalPml3Base = LocalPml2Base + ((uint64_t)LocalFractalIndex << 21);
        //  Start of the active PML1 tables (PTs).
        static constexpr uintptr_t const LocalPml4Base = LocalPml3Base + ((uint64_t)LocalFractalIndex << 12);

        //  Start of the temporary PML4 tables.
        static constexpr uintptr_t const AlienPml1Base = 0xFFFF000000000000ULL + ((uint64_t)AlienFractalIndex << 39);
        //  Start of the active PML3 tables (PDPTs).
        static constexpr uintptr_t const AlienPml2Base = AlienPml1Base + ((uint64_t)LocalFractalIndex << 30);
        //  Start of the active PML2 tables (PDs).
        static constexpr uintptr_t const AlienPml3Base = AlienPml2Base + ((uint64_t)LocalFractalIndex << 21);
        //  Start of the active PML1 tables (PTs).
        static constexpr uintptr_t const AlienPml4Base = LocalPml3Base + ((uint64_t)AlienFractalIndex << 12);

        static constexpr uintptr_t const FractalStart = AlienPml1Base;
        static constexpr uintptr_t const FractalEnd   = FractalStart + (2ULL << 39);
        //  The pages in this 1-TiB range are automagically allocated due
        //  to the awesome fractal mapping! :D

        static constexpr size_t const RecursiveUnmapDepth = 32;

        /*  Constructor(s)  */

    protected:
        VmmArc() = default;

    public:
        VmmArc(VmmArc const &) = delete;
        VmmArc & operator =(VmmArc const &) = delete;

        /*  Static Translation Methods  */

        static __forceinline uint16_t GetPml4Index(vaddr_t const addr)
        {
            return (uint16_t)((addr.Value >> 39) & 511);
        }

        static __forceinline uint16_t GetPml3Index(vaddr_t const addr)
        {
            return (uint16_t)((addr.Value >> 30) & 511);
        }

        static __forceinline uint16_t GetPml2Index(vaddr_t const addr)
        {
            return (uint16_t)((addr.Value >> 21) & 511);
        }

        static __forceinline uint16_t GetPml1Index(vaddr_t const addr)
        {
            return (uint16_t)((addr.Value >> 12) & 511);
        }

        //  Local Map.

        static __forceinline Pml4 * GetLocalPml4()          { return (Pml4 *)LocalPml4Base; }
        static __forceinline Pml4 * GetLocalPml4Ex(vaddr_t) { return (Pml4 *)LocalPml4Base; }
        static __forceinline Pml4Entry & GetLocalPml4Entry(vaddr_t const addr)
        {
            return (*GetLocalPml4())[GetPml4Index(addr)];
        }

        static __forceinline Pml3 * GetLocalPml3(vaddr_t const addr)
        {
            return (Pml3 *)(LocalPml3Base + ((addr.Value >> 27) & 0x00000000001FF000ULL));
        }
        static __forceinline Pml3Entry & GetLocalPml3Entry(vaddr_t const addr)
        {
            return (*GetLocalPml3(addr))[GetPml3Index(addr)];
        }

        static __forceinline Pml2 * GetLocalPml2(vaddr_t const addr)
        {
            return (Pml2 *)(LocalPml2Base + ((addr.Value >> 18) & 0x000000003FFFF000ULL));
        }
        static __forceinline Pml2Entry & GetLocalPml2Entry(vaddr_t const addr)
        {
            return (*GetLocalPml2(addr))[GetPml2Index(addr)];
        }

        static __forceinline Pml1 * GetLocalPml1(vaddr_t const addr)
        {
            return (Pml1 *)(LocalPml1Base + ((addr.Value >>  9) & 0x0000007FFFFFF000ULL));
        }
        static __forceinline Pml1Entry & GetLocalPml1Entry(vaddr_t const addr)
        {
            return (*GetLocalPml1(addr))[GetPml1Index(addr)];
        }

        //  Alien Map.

        static __forceinline Pml4 * GetAlienPml4()          { return (Pml4 *)AlienPml4Base; }
        static __forceinline Pml4 * GetAlienPml4Ex(vaddr_t) { return (Pml4 *)AlienPml4Base; }
        static __forceinline Pml4Entry & GetAlienPml4Entry(vaddr_t const addr)
        {
            return (*GetAlienPml4())[GetPml4Index(addr)];
        }

        static __forceinline Pml3 * GetAlienPml3(vaddr_t const addr)
        {
            return (Pml3 *)(AlienPml3Base + ((addr.Value >> 27) & 0x00000000001FF000ULL));
        }
        static __forceinline Pml3Entry & GetAlienPml3Entry(vaddr_t const addr)
        {
            return (*GetAlienPml3(addr))[GetPml3Index(addr)];
        }

        static __forceinline Pml2 * GetAlienPml2(vaddr_t const addr)
        {
            return (Pml2 *)(AlienPml2Base + ((addr.Value >> 18) & 0x000000003FFFF000ULL));
        }
        static __forceinline Pml2Entry & GetAlienPml2Entry(vaddr_t const addr)
        {
            return (*GetAlienPml2(addr))[GetPml2Index(addr)];
        }

        static __forceinline Pml1 * GetAlienPml1(vaddr_t const addr)
        {
            return (Pml1 *)(AlienPml1Base + ((addr.Value >>  9) & 0x0000007FFFFFF000ULL));
        }
        static __forceinline Pml1Entry & GetAlienPml1Entry(vaddr_t const addr)
        {
            return (*GetAlienPml1(addr))[GetPml1Index(addr)];
        }
    };
}}
