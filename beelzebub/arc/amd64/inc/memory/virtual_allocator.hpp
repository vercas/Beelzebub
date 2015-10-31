/**
 *  Some of the code is inspired from:
 *  http://forum.osdev.org/viewtopic.php?f=15&t=25545
 */

#pragma once

#include <memory/paging.hpp>
#include <memory/page_allocator.hpp>
#include <memory/manager.hpp>
#include <system/cpu.hpp>
#include <system/cpuid.hpp>

#include <stdlib/iterator.hpp>

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

        static __bland inline uint16_t GetPml4Index(vaddr_t const addr)
        {
            return (uint16_t)((addr >> 39) & 511);
        }

        static __bland inline uint16_t GetPml3Index(vaddr_t const addr)
        {
            return (uint16_t)((addr >> 30) & 511);
        }

        static __bland inline uint16_t GetPml2Index(vaddr_t const addr)
        {
            return (uint16_t)((addr >> 21) & 511);
        }

        static __bland inline uint16_t GetPml1Index(vaddr_t const addr)
        {
            return (uint16_t)((addr >> 12) & 511);
        }

        //  Local Map.

        static __bland inline Pml4 * const GetLocalPml4()          { return (Pml4 *)LocalPml4Base; }
        static __bland inline Pml4 * const GetLocalPml4Ex(vaddr_t) { return (Pml4 *)LocalPml4Base; }

        static __bland inline Pml3 * const GetLocalPml3(vaddr_t const addr)
        {
            return (Pml3 *)(LocalPml3Base + ((addr >> 27) & 0x00000000001FF000ULL));
        }

        static __bland inline Pml2 * const GetLocalPml2(vaddr_t const addr)
        {
            return (Pml2 *)(LocalPml2Base + ((addr >> 18) & 0x000000003FFFF000ULL));
        }

        static __bland inline Pml1 * const GetLocalPml1(vaddr_t const addr)
        {
            return (Pml1 *)(LocalPml1Base + ((addr >>  9) & 0x0000007FFFFFF000ULL));
        }

        static __bland inline Pml1Entry & GetLocalPml1Entry(vaddr_t const addr)
        {
            return (*GetLocalPml1(addr))[GetPml1Index(addr)];
        }

        //  Alien Map.

        static __bland inline Pml4 * const GetAlienPml4()          { return (Pml4 *)AlienPml4Base; }
        static __bland inline Pml4 * const GetAlienPml4Ex(vaddr_t) { return (Pml4 *)AlienPml4Base; }

        static __bland inline Pml3 * const GetAlienPml3(vaddr_t const addr)
        {
            return (Pml3 *)(AlienPml3Base + ((addr >> 27) & 0x00000000001FF000ULL));
        }

        static __bland inline Pml2 * const GetAlienPml2(vaddr_t const addr)
        {
            return (Pml2 *)(AlienPml2Base + ((addr >> 18) & 0x000000003FFFF000ULL));
        }

        static __bland inline Pml1 * const GetAlienPml1(vaddr_t const addr)
        {
            return (Pml1 *)(AlienPml1Base + ((addr >>  9) & 0x0000007FFFFFF000ULL));
        }

        static __bland inline Pml1Entry & GetAlienPml1Entry(vaddr_t const addr)
        {
            return (*GetAlienPml1(addr))[GetPml1Index(addr)];
        }

        /*  Constructors  */

        VirtualAllocationSpace() = default;
        VirtualAllocationSpace(VirtualAllocationSpace const &) = delete;
        VirtualAllocationSpace & operator =(VirtualAllocationSpace const &) = delete;

        __bland inline explicit VirtualAllocationSpace(PageAllocator * const allocator)
            : Allocator( allocator)
            , Pml4Address(nullpaddr)
        {

        }

        /*  Setup  */

        __cold __bland Handle Bootstrap(System::CpuId const * const bspcpuid);

        __bland Handle Clone(VirtualAllocationSpace * const target);

        /*  Main Operations  */

        __bland __forceinline void Activate() const
        {
            const Cr3 newVal = Cr3(this->Pml4Address, false, false);

            Cpu::SetCr3(newVal);
        }

        __bland __forceinline void Alienate() const
        {
            Pml4 & pml4 = *GetLocalPml4();

            pml4[AlienFractalIndex] = Pml4Entry(this->Pml4Address, true, true, false, NX);
        }

        __bland __forceinline bool IsLocal() const
        {
            Pml4 & pml4 = *GetLocalPml4();

            return pml4[LocalFractalIndex].GetPml3Address() == this->Pml4Address;
        }

        __bland __forceinline bool IsAlien() const
        {
            Pml4 & pml4 = *GetLocalPml4();

            return pml4[AlienFractalIndex].GetPml3Address() == this->Pml4Address;
        }

        /*  Translation  */

        //  Translates the address with the current VAS.
        static __bland __forceinline paddr_t TranslateLocal(vaddr_t const vaddr)
        {
            return GetLocalPml1Entry(vaddr).GetAddress() + (paddr_t)(vaddr & 0xFFF);
            //  Yeah, the offset within the page is preserved.
        }

        template<typename cbk_t>
        __hot __bland Handle TryTranslate(vaddr_t const vaddr, cbk_t cbk, bool const tolerate);
        __hot __bland Handle GetEntry(vaddr_t const vaddr, Pml1Entry * & e, bool const tolerate);

        /*  Mapping  */

        __hot __bland __noinline Handle Map(vaddr_t const vaddr, paddr_t const paddr, PageFlags const flags, PageDescriptor * & pml3desc, PageDescriptor * & pml2desc, PageDescriptor * & pml1desc);
        __hot __bland __noinline Handle Map(vaddr_t const vaddr, paddr_t const paddr, PageFlags const flags);
        __hot __bland __noinline Handle Unmap(vaddr_t const vaddr, paddr_t & paddr);

        /*  Flags  */

        __bland Handle GetPageFlags(vaddr_t const vaddr, PageFlags & flags);
        __bland Handle SetPageFlags(vaddr_t const vaddr, PageFlags const flags);

        /*  Fields  */

        PageAllocator * Allocator;
        //psize_t FreePagesCount, MappedPagesCount;

        paddr_t Pml4Address;

        /*  Traversal  */

        struct Iterator
            : public Std::Iterator<Std::RandomAccessIteratorTag, Pml1Entry>
        {

            /*  Constructor(s)  */

            Iterator() = default;
            Iterator(Iterator const &) = default;
            Iterator & operator =(Iterator const &) = default;

            static __bland Handle Create(Iterator & dst, VirtualAllocationSpace * const space, vaddr_t const vaddr);

        private:

            __hot __bland Handle Initialize();

            __bland inline Iterator(VirtualAllocationSpace * const space, vaddr_t const vaddr)
                : AllocationSpace( space )
                , VirtualAddress(vaddr)
            {
                
            }

        public:

            /*  Methods  */

            __bland __forceinline Pml1Entry * GetEntry() const
            {
                return this->Entry;
            }
            __bland __forceinline bool GetTablesPresent() const
            {
                return this->Entry != nullptr;
            }

            __hot __bland Handle AllocateTables(PageDescriptor * & pml3desc, PageDescriptor * & pml2desc, PageDescriptor * & pml1desc);

            __hot __bland __forceinline Handle AllocateTables()
            {
                PageDescriptor * desc;
                //  Swallow all the descriptors.

                return this->AllocateTables(desc, desc, desc);
            }

            /*  Operators  */

            __bland               const Iterator  & operator +=(DifferenceType const diff);

            __bland __forceinline const Iterator  & operator -=(DifferenceType const diff)
            {
                return (*this) += -diff;
            }

            __bland               const Iterator    operator  +(DifferenceType const diff);

            __bland __forceinline const Iterator    operator  -(DifferenceType const diff)
            {
                return (*this) + (-diff);
            }

            __bland __forceinline const Iterator  & operator ++()
            {
                return (*this) += 1;
            }
            __bland __forceinline const Iterator  & operator ++(int)
            {
                return (*this) += 1;
            }
            __bland __forceinline const Iterator  & operator --()
            {
                return (*this) -= 1;
            }
            __bland __forceinline const Iterator  & operator --(int)
            {
                return (*this) -= 1;
            }

            __bland __forceinline       ValueType & operator [](DifferenceType const index)
            {
                return *((*this) + index);
            }
            __bland __forceinline       ValueType & operator  *() const
            {
                return *this->GetEntry();
            }

            /*  Fields  */

        private:

            VirtualAllocationSpace * AllocationSpace;
            vaddr_t VirtualAddress;

            Pml1Entry * Entry;
        };

        __hot __bland inline Handle GetIterator(Iterator & dst, vaddr_t const vaddr)
        {
            return Iterator::Create(dst, this, vaddr);
        }
    };
}}
