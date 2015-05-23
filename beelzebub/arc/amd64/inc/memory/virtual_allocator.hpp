/**
 *  Some of the code is inspired from:
 *  http://forum.osdev.org/viewtopic.php?f=15&t=25545
 */

#pragma once

#include <kernel.hpp>
#include <terminals/base.hpp>
#include <arc/synchronization/spinlock.hpp>
#include <memory/page_allocator.hpp>
#include <memory/allocation.hpp>
#include <arc/memory/paging.hpp>
#include <handles.h>
#include <metaprogramming.h>

using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Memory::Paging;

namespace Beelzebub { namespace Memory
{
    /**
     *  Manages assignment and allocation of virtual (linear) memory pages.
     */
    class VirtualAllocationSpace
    {
        /*  Private Constants  */

        //  Start of the higher half address.
        static const vaddr_t HigherHalf = 0xFFFF000000000000ULL;

        //  Right below the kernel.
        static const uint16_t FractalIndex = 510;

        //  Start of the PML4 tables.
        static const vaddr_t Pml4Base = HigherHalf + ((vaddr_t)FractalIndex << 12);
        //  Start of the PML3 tables (PDPTs).
        static const vaddr_t Pml3Base = HigherHalf + ((vaddr_t)FractalIndex << 21);
        //  Start of the PML2 tables (PDs).
        static const vaddr_t Pml2Base = HigherHalf + ((vaddr_t)FractalIndex << 30);
        //  Start of the PML1 tables (PTs).
        static const vaddr_t Pml1Base = HigherHalf + ((vaddr_t)FractalIndex << 39);

        /*  Cached Feature Flags  */

        static bool Page1GB, NX;

    public:

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

        static __bland __forceinline Pml4 * const GetPml4() { return (Pml4 *)Pml4Base; }

        static __bland __forceinline Pml3 * const GetPml3(const vaddr_t addr)
        {
            return (Pml3 *)(Pml3Base + ((addr >> 27) & 0x00000000001FF000ULL));
        }

        static __bland __forceinline Pml2 * const GetPml2(const vaddr_t addr)
        {
            return (Pml2 *)(Pml2Base + ((addr >> 18) & 0x000000003FFFF000ULL));
        }

        static __bland __forceinline Pml1 * const GetPml1(const vaddr_t addr)
        {
            return (Pml1 *)(Pml1Base + ((addr >>  9) & 0x0000007FFFFFF000ULL));
        }

        static __bland __forceinline Pml1Entry & GetPml1Entry(const vaddr_t addr)
        {
            return (*GetPml1(addr))[GetPml1Index(addr)];
        }

        //  Translates the address with the current VAS.
        static __bland __forceinline paddr_t Translate(const vaddr_t address)
        {
            return GetPml1Entry(address).GetAddress() + (paddr_t)(address & 4095);
            //  Yeah, the offset within the page is preserved.
        }

        /*  Constructor  */

        VirtualAllocationSpace() = default;
        VirtualAllocationSpace(VirtualAllocationSpace const&) = default;

        __bland VirtualAllocationSpace(PageAllocator * const allocator);

        /*  Main Operations  */

        __bland Handle Bootstrap();
        __bland VirtualAllocationSpace * Clone();
        __bland Handle Activate();

        /*  Fields  */

        PageAllocator * Allocator;
        psize_t FreePagesCount, MappedPagesCount;

        paddr_t Pml4Address;

        bool Active;
    };
}}
