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
 *  Some of the code is inspired from:
 *  http://forum.osdev.org/viewtopic.php?f=15&t=25545
 */

#pragma once

#include "memory/paging.hpp"
#include "memory/page_allocator.hpp"
#include "memory/manager.hpp"
#include "system/cpuid.hpp"

#include "stdlib/iterator.hpp"

using namespace Beelzebub::System;

namespace Beelzebub { namespace Memory
{
    typedef Handle (*VasTranslationCallback)(Pml1Entry * e);

    /**
     *  Manages assignment and allocation of virtual (linear) memory pages.
     */
    class VirtualAllocationSpace
    {
    public:

        /*  Cached Feature Flags  */

        static bool Page1GB, NX;

        /*  Public Constants  */

        //  End of the lower half address range.
        static vaddr_t const LowerHalfEnd              = 0x0001000000000000ULL;
        //  Start of the higher half address range.
        static vaddr_t const HigherHalfStart           = 0xFFFF800000000000ULL;

        //  Start of the kernel area.
        static vaddr_t const KernelStart               = 0xFFFFFF8000000000ULL;
        //  Start of allocation space control structures.
        static vaddr_t const PasControlStructuresStart = 0xFFFF808000000000ULL;
        //  End of allocation space control strucutres.
        static vaddr_t const PasControlStructuresEnd   = 0xFFFF820000000000ULL;

        //  Right below the kernel.
        static uint16_t const LocalFractalIndex = 510;
        //  Right below the kernel.
        static uint16_t const AlienFractalIndex = 509;

        //  Start of the active PML4 tables.
        static vaddr_t const LocalPml1Base = 0xFFFF000000000000ULL + ((vaddr_t)LocalFractalIndex << 39);
        //  Start of the active PML3 tables (PDPTs).
        static vaddr_t const LocalPml2Base = LocalPml1Base         + ((vaddr_t)LocalFractalIndex << 30);
        //  Start of the active PML2 tables (PDs).
        static vaddr_t const LocalPml3Base = LocalPml2Base         + ((vaddr_t)LocalFractalIndex << 21);
        //  Start of the active PML1 tables (PTs).
        static vaddr_t const LocalPml4Base = LocalPml3Base         + ((vaddr_t)LocalFractalIndex << 12);

        //  Start of the temporary PML4 tables.
        static vaddr_t const AlienPml1Base = 0xFFFF000000000000ULL + ((vaddr_t)AlienFractalIndex << 39);
        //  Start of the active PML3 tables (PDPTs).
        static vaddr_t const AlienPml2Base = AlienPml1Base         + ((vaddr_t)LocalFractalIndex << 30);
        //  Start of the active PML2 tables (PDs).
        static vaddr_t const AlienPml3Base = AlienPml2Base         + ((vaddr_t)LocalFractalIndex << 21);
        //  Start of the active PML1 tables (PTs).
        //static vaddr_t const AlienPml4Base = AlienPml3Base         + ((vaddr_t)LocalFractalIndex << 12);
        static vaddr_t const AlienPml4Base = LocalPml3Base         + ((vaddr_t)AlienFractalIndex << 12);

        static vaddr_t const FractalStart = LocalPml1Base;
        static vaddr_t const FractalEnd   = FractalStart + (1ULL << 39);
        //  The pages in this 512-GiB range are automagically allocated due
        //  to the awesome fractal mapping! :D

        /*  Static Translation Methods  */

        static inline uint16_t GetPml4Index(vaddr_t const addr)
        {
            return (uint16_t)((addr >> 39) & 511);
        }

        static inline uint16_t GetPml3Index(vaddr_t const addr)
        {
            return (uint16_t)((addr >> 30) & 511);
        }

        static inline uint16_t GetPml2Index(vaddr_t const addr)
        {
            return (uint16_t)((addr >> 21) & 511);
        }

        static inline uint16_t GetPml1Index(vaddr_t const addr)
        {
            return (uint16_t)((addr >> 12) & 511);
        }

        //  Local Map.

        static inline Pml4 * const GetLocalPml4()          { return (Pml4 *)LocalPml4Base; }
        static inline Pml4 * const GetLocalPml4Ex(vaddr_t) { return (Pml4 *)LocalPml4Base; }

        static inline Pml3 * const GetLocalPml3(vaddr_t const addr)
        {
            return (Pml3 *)(LocalPml3Base + ((addr >> 27) & 0x00000000001FF000ULL));
        }

        static inline Pml2 * const GetLocalPml2(vaddr_t const addr)
        {
            return (Pml2 *)(LocalPml2Base + ((addr >> 18) & 0x000000003FFFF000ULL));
        }

        static inline Pml1 * const GetLocalPml1(vaddr_t const addr)
        {
            return (Pml1 *)(LocalPml1Base + ((addr >>  9) & 0x0000007FFFFFF000ULL));
        }

        static inline Pml1Entry & GetLocalPml1Entry(vaddr_t const addr)
        {
            return (*GetLocalPml1(addr))[GetPml1Index(addr)];
        }

        //  Alien Map.

        static inline Pml4 * const GetAlienPml4()          { return (Pml4 *)AlienPml4Base; }
        static inline Pml4 * const GetAlienPml4Ex(vaddr_t) { return (Pml4 *)AlienPml4Base; }

        static inline Pml3 * const GetAlienPml3(vaddr_t const addr)
        {
            return (Pml3 *)(AlienPml3Base + ((addr >> 27) & 0x00000000001FF000ULL));
        }

        static inline Pml2 * const GetAlienPml2(vaddr_t const addr)
        {
            return (Pml2 *)(AlienPml2Base + ((addr >> 18) & 0x000000003FFFF000ULL));
        }

        static inline Pml1 * const GetAlienPml1(vaddr_t const addr)
        {
            return (Pml1 *)(AlienPml1Base + ((addr >>  9) & 0x0000007FFFFFF000ULL));
        }

        static inline Pml1Entry & GetAlienPml1Entry(vaddr_t const addr)
        {
            return (*GetAlienPml1(addr))[GetPml1Index(addr)];
        }

        /*  Constructors  */

        VirtualAllocationSpace() = default;
        VirtualAllocationSpace(VirtualAllocationSpace const &) = delete;
        VirtualAllocationSpace & operator =(VirtualAllocationSpace const &) = delete;

        inline explicit VirtualAllocationSpace(PageAllocator * const allocator)
            : Allocator( allocator)
            , Pml4Address(nullpaddr)
            //, AlienPml4Lock()
        {

        }

        /*  Setup  */

        __cold Handle Bootstrap(System::CpuId const * const bspcpuid);

        Handle Clone(VirtualAllocationSpace * const target);

        /*  Main Operations  */

        __hot void Activate() const;

        __forceinline void Alienate() const
        {
            Pml4 & pml4 = *GetLocalPml4();

            pml4[AlienFractalIndex] = Pml4Entry(this->Pml4Address, true, true, false, NX);
        }

        __forceinline bool IsLocal() const
        {
            Pml4 & pml4 = *GetLocalPml4();

            return pml4[LocalFractalIndex].GetPml3Address() == this->Pml4Address;
        }

        __forceinline bool IsAlien() const
        {
            Pml4 & pml4 = *GetLocalPml4();

            return pml4[AlienFractalIndex].GetPml3Address() == this->Pml4Address;
        }

        /*  Translation  */

        //  Translates the address with the current VAS.
        static __forceinline paddr_t TranslateLocal(vaddr_t const vaddr)
        {
            return GetLocalPml1Entry(vaddr).GetAddress() + (paddr_t)(vaddr & (PageSize - 1));
            //  Yeah, the offset within the page is preserved.
        }

        template<typename cbk_t>
        __hot Handle TryTranslate(vaddr_t const vaddr, cbk_t cbk, bool const tolerate);
        __hot Handle GetEntry(vaddr_t const vaddr, Pml1Entry * & e, bool const tolerate);

        /*  Mapping  */

        __hot __noinline Handle Map(vaddr_t const vaddr, paddr_t const paddr, PageFlags const flags, PageDescriptor * & pml3desc, PageDescriptor * & pml2desc, PageDescriptor * & pml1desc);
        __hot __noinline Handle Map(vaddr_t const vaddr, paddr_t const paddr, PageFlags const flags);
        __hot __noinline Handle Unmap(vaddr_t const vaddr, paddr_t & paddr);

        /*  Flags  */

        Handle GetPageFlags(vaddr_t const vaddr, PageFlags & flags);
        Handle SetPageFlags(vaddr_t const vaddr, PageFlags const flags);

        /*  Fields  */

        PageAllocator * Allocator;
        //psize_t FreePagesCount, MappedPagesCount;

        paddr_t Pml4Address;

        //Synchronization::Spinlock<> AlienPml4Lock;

        /*  Traversal  */

        struct Iterator
            : public Std::Iterator<Std::RandomAccessIteratorTag, Pml1Entry>
        {

            /*  Constructor(s)  */

            Iterator() = default;
            Iterator(Iterator const &) = default;
            Iterator & operator =(Iterator const &) = default;

            static Handle Create(Iterator & dst, VirtualAllocationSpace * const space, vaddr_t const vaddr);

        private:

            __hot Handle Initialize();

            inline Iterator(VirtualAllocationSpace * const space, vaddr_t const vaddr)
                : AllocationSpace( space )
                , VirtualAddress(vaddr)
            {
                
            }

        public:

            /*  Methods  */

            __forceinline Pml1Entry * GetEntry() const
            {
                return this->Entry;
            }
            __forceinline bool GetTablesPresent() const
            {
                return this->Entry != nullptr;
            }

            __hot Handle AllocateTables(PageDescriptor * & pml3desc, PageDescriptor * & pml2desc, PageDescriptor * & pml1desc);

            __hot __forceinline Handle AllocateTables()
            {
                PageDescriptor * desc;
                //  Swallow all the descriptors.

                return this->AllocateTables(desc, desc, desc);
            }

            /*  Operators  */

            const Iterator  & operator +=(DifferenceType const diff);

            __forceinline const Iterator  & operator -=(DifferenceType const diff)
            {
                return (*this) += -diff;
            }

            const Iterator    operator  +(DifferenceType const diff);

            __forceinline const Iterator    operator  -(DifferenceType const diff)
            {
                return (*this) + (-diff);
            }

            __forceinline const Iterator  & operator ++()
            {
                return (*this) += 1;
            }
            __forceinline const Iterator  & operator ++(int)
            {
                return (*this) += 1;
            }
            __forceinline const Iterator  & operator --()
            {
                return (*this) -= 1;
            }
            __forceinline const Iterator  & operator --(int)
            {
                return (*this) -= 1;
            }

            __forceinline       ValueType & operator [](DifferenceType const index)
            {
                return *((*this) + index);
            }
            __forceinline       ValueType & operator  *() const
            {
                return *this->GetEntry();
            }

            /*  Fields  */

        private:

            VirtualAllocationSpace * AllocationSpace;
            vaddr_t VirtualAddress;

            Pml1Entry * Entry;
        };

        __hot inline Handle GetIterator(Iterator & dst, vaddr_t const vaddr)
        {
            return Iterator::Create(dst, this, vaddr);
        }
    };
}}
