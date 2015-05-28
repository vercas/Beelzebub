/**
 *  Some of the code is inspired from:
 *  http://forum.osdev.org/viewtopic.php?f=15&t=25545
 */

#pragma once

#include <kernel.hpp>
#include <terminals/base.hpp>
#include <synchronization/spinlock.hpp>
#include <memory/page_allocator.hpp>
#include <memory/allocation.hpp>
#include <memory/paging.hpp>
#include <system/cpu.hpp>
#include <handles.h>
#include <metaprogramming.h>

using namespace Beelzebub::System;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Memory::Paging;

namespace Beelzebub { namespace Memory
{
    /**
     *  Manages assignment and allocation of virtual (linear) memory pages.
     */
    class VirtualAllocationSpace
    {
        /*  Cached Feature Flags  */

        static bool Page1GB, NX;

    public:

        /*  Public Constants  */

        //  End of the lower half address range.
        static const vaddr_t LowerHalfEnd              = 0x0001000000000000ULL;
        //  Start of the higher half address range.
        static const vaddr_t HigherHalfStart           = 0xFFFF800000000000ULL;

        //  Start of the kernel area.
        static const vaddr_t KernelStart               = 0xFFFFFF8000000000ULL;
        //  Start of allocation space control structures.
        static const vaddr_t PasControlStructuresStart = 0xFFFF808000000000ULL;
        //  End of allocation space control strucutres.
        static const vaddr_t PasControlStructuresEnd   = 0xFFFF820000000000ULL;

        //  Right below the kernel.
        static const uint16_t LocalFractalIndex = 510;
        //  Right below the kernel.
        static const uint16_t AlienFractalIndex = 509;

        //  Start of the active PML4 tables.
        static const vaddr_t LocalPml1Base = 0xFFFF000000000000ULL + ((vaddr_t)LocalFractalIndex << 39);
        //  Start of the active PML3 tables (PDPTs).
        static const vaddr_t LocalPml2Base = LocalPml1Base         + ((vaddr_t)LocalFractalIndex << 30);
        //  Start of the active PML2 tables (PDs).
        static const vaddr_t LocalPml3Base = LocalPml2Base         + ((vaddr_t)LocalFractalIndex << 21);
        //  Start of the active PML1 tables (PTs).
        static const vaddr_t LocalPml4Base = LocalPml3Base         + ((vaddr_t)LocalFractalIndex << 12);

        //  Start of the temporary PML4 tables.
        static const vaddr_t AlienPml1Base = 0xFFFF000000000000ULL + ((vaddr_t)LocalFractalIndex << 39);
        //  Start of the active PML3 tables (PDPTs).
        static const vaddr_t AlienPml2Base = AlienPml1Base         + ((vaddr_t)LocalFractalIndex << 30);
        //  Start of the active PML2 tables (PDs).
        static const vaddr_t AlienPml3Base = AlienPml2Base         + ((vaddr_t)LocalFractalIndex << 21);
        //  Start of the active PML1 tables (PTs).
        static const vaddr_t AlienPml4Base = AlienPml3Base         + ((vaddr_t)LocalFractalIndex << 12);

        static const vaddr_t FractalStart = LocalPml1Base;
        static const vaddr_t FractalEnd   = FractalStart + (1ULL << 39);
        //  The pages in this 512-GiB range are automagically allocated due
        //  to the awesome fractal mapping! :D

        /*  Static Translation Methods  */

        static __bland __forceinline uint16_t GetPml4Index(const vaddr_t addr)
        {
            return (uint16_t)((addr >> 39) & 511);
        }

        static __bland __forceinline uint16_t GetPml3Index(const vaddr_t addr)
        {
            return (uint16_t)((addr >> 30) & 511);
        }

        static __bland __forceinline uint16_t GetPml2Index(const vaddr_t addr)
        {
            return (uint16_t)((addr >> 21) & 511);
        }

        static __bland __forceinline uint16_t GetPml1Index(const vaddr_t addr)
        {
            return (uint16_t)((addr >> 12) & 511);
        }

        //  Local Map.

        static __bland __forceinline Pml4 * const GetLocalPml4() { return (Pml4 *)LocalPml4Base; }

        static __bland __forceinline Pml3 * const GetLocalPml3(const vaddr_t addr)
        {
            return (Pml3 *)(LocalPml3Base + ((addr >> 27) & 0x00000000001FF000ULL));
        }

        static __bland __forceinline Pml2 * const GetLocalPml2(const vaddr_t addr)
        {
            return (Pml2 *)(LocalPml2Base + ((addr >> 18) & 0x000000003FFFF000ULL));
        }

        static __bland __forceinline Pml1 * const GetLocalPml1(const vaddr_t addr)
        {
            return (Pml1 *)(LocalPml1Base + ((addr >>  9) & 0x0000007FFFFFF000ULL));
        }

        static __bland __forceinline Pml1Entry & GetLocalPml1Entry(const vaddr_t addr)
        {
            return (*GetLocalPml1(addr))[GetPml1Index(addr)];
        }

        //  Alien Map.

        static __bland __forceinline Pml4 * const GetAlienPml4() { return (Pml4 *)AlienPml4Base; }

        static __bland __forceinline Pml3 * const GetAlienPml3(const vaddr_t addr)
        {
            return (Pml3 *)(AlienPml3Base + ((addr >> 27) & 0x00000000001FF000ULL));
        }

        static __bland __forceinline Pml2 * const GetAlienPml2(const vaddr_t addr)
        {
            return (Pml2 *)(AlienPml2Base + ((addr >> 18) & 0x000000003FFFF000ULL));
        }

        static __bland __forceinline Pml1 * const GetAlienPml1(const vaddr_t addr)
        {
            return (Pml1 *)(AlienPml1Base + ((addr >>  9) & 0x0000007FFFFFF000ULL));
        }

        static __bland __forceinline Pml1Entry & GetAlienPml1Entry(const vaddr_t addr)
        {
            return (*GetAlienPml1(addr))[GetPml1Index(addr)];
        }

        //  Translation.

        //  Translates the address with the current VAS.
        static __bland __forceinline paddr_t TranslateLocal(const vaddr_t address)
        {
            return GetLocalPml1Entry(address).GetAddress() + (paddr_t)(address & 4095);
            //  Yeah, the offset within the page is preserved.
        }

        /*  Constructor  */

        VirtualAllocationSpace() = default;
        VirtualAllocationSpace(VirtualAllocationSpace const &) = delete;
        VirtualAllocationSpace & operator =(const VirtualAllocationSpace &) = delete;

        __bland VirtualAllocationSpace(PageAllocator * const allocator);

        /*  Main Operations  */

        __cold __bland Handle Bootstrap();
        __bland Handle Clone(VirtualAllocationSpace * const target);

        __bland __forceinline void Activate()
        {
            const Cr3 newVal = Cr3(this->Pml4Address, false, false);

            Cpu::SetCr3(newVal);
        }

        __bland __forceinline void Alienate()
        {
            Pml4 & pml4 = *GetLocalPml4();

            pml4[AlienFractalIndex] = Pml4Entry(this->Pml4Address, true, true, false, NX);
        }

        __bland __forceinline bool IsLocal() const
        {
            Pml4 & pml4 = *GetLocalPml4();

            return pml4[LocalFractalIndex].GetAddress() == this->Pml4Address;
        }

        __bland __forceinline bool IsAlien() const
        {
            Pml4 & pml4 = *GetLocalPml4();

            return pml4[AlienFractalIndex].GetAddress() == this->Pml4Address;
        }

        /*  Mapping  */

        __hot __bland Handle Map(const vaddr_t vaddr, const paddr_t paddr, const PageFlags flags);
        __hot __bland Handle Unmap(const vaddr_t vaddr);

        /*  Fields  */

        PageAllocator * Allocator;
        //psize_t FreePagesCount, MappedPagesCount;

        paddr_t Pml4Address;
    };
}}
