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

#include <synchronization/spinlock_uninterruptible.hpp>
#include <synchronization/atomic.hpp>
// #include <terminals/base.hpp>
#include <beel/handles.h>

namespace Beelzebub { namespace Memory
{
    /**
     * Represents possible options for memory page allocation.
     */
    enum class PageAllocationOptions : int
    {
        //  64-bit pages preferred.
        GeneralPages   = 0,
        //  32-bit pages mandatory.
        ThirtyTwoBit   = 1 << 0,
    };

    ENUMOPS(PageAllocationOptions, int)

    /**
     * Represents possible options for memory page reservation.
     */
    enum class PageReservationOptions : int
    {
        //  Only free pages will be reserved.
        OnlyFree       = 0,
        //  Caching pages will be reserved as well.
        IncludeCaching = 1 << 0,
        //  In-use pages will be reserved as well.
        IncludeInUse   = 1 << 1,
        //  Pages that are already reserved will be ignored.
        IgnoreReserved = 1 << 2,
    };

    ENUMOPS(PageReservationOptions, int)

    /**
     * Represents possible statuses of a memory page.
     */
    enum class PageDescriptorStatus : uint16_t
    {
        Free     =  0,
        Caching  =  1,
        InUse    =  2,
        Reserved =  3,
    };

    /**
     * Describes a page of memory.
     */
    struct PageDescriptor
    {
        /*  Constants  */

        static constexpr PageDescriptor * const Invalid
        = reinterpret_cast<PageDescriptor *>(0x42);

        static __forceinline bool IsValid(PageDescriptor const * const desc)
        {
            return desc != Invalid;
        }

        static const uint16_t MaxAccesses = (uint16_t)0xFFFF;

        /*  Fields  */

        //  Index of the page in the allocation stack.
        pgind_t StackIndex;

        //  Number of references to this page.
        Synchronization::Atomic<uint32_t> ReferenceCount;

        //  Page status
        PageDescriptorStatus Status;
        //  Access count
        Synchronization::Atomic<uint16_t> Accesses;

        /*  Constructors  */

        PageDescriptor() = default;
        PageDescriptor(PageDescriptor const &) = delete;
        PageDescriptor & operator =(PageDescriptor const &) = delete;

        __forceinline PageDescriptor(const uint64_t stackIndex)
            : StackIndex( stackIndex )
            , ReferenceCount(0)
            , Status(PageDescriptorStatus::Free)
            , Accesses(0)
        {

        }

        __forceinline PageDescriptor(const uint64_t stackIndex
                                           , const PageDescriptorStatus status)
            : StackIndex( stackIndex )
            , ReferenceCount(0)
            , Status(status)
            , Accesses(0)
        {

        }

        /*  Accesses  */

        __forceinline void ResetAccesses()
        {
            this->Accesses = 0;
        }

        __forceinline uint16_t IncrementAccesses()
        {
            //  An exact count isn't really required, but it may
            //  prove useful.

            uint16_t ret = ++this->Accesses;

            if unlikely(ret == 0)
            {
                this->Accesses.CmpXchgStrong(ret, 1);
                //  If it's 0, set to 1.

                return 1;
            }
            else
                return ret;
        }

        /*  Reference count  */

        __forceinline void ResetReferenceCount()
        {
            this->ReferenceCount = 0;
            //  This really should be atomic...
        }

        __forceinline uint32_t IncrementReferenceCount()
        {
            return ++this->ReferenceCount;
        }

        uint32_t DecrementReferenceCount();

        /*  Status  */

        __forceinline void Free()
        {
            this->Status = PageDescriptorStatus::Free;

            this->ResetAccesses();
            this->ResetReferenceCount();
        }

        __forceinline void Use()
        {
            this->Status = PageDescriptorStatus::InUse;

            this->ResetAccesses();
            this->ResetReferenceCount();
        }

        __forceinline void Reserve()
        {
            this->Status = PageDescriptorStatus::Reserved;

            this->ResetReferenceCount();
        }

        /*  Debug  */

#ifdef __BEELZEBUB__DEBUG
        __forceinline const char * GetStatusString() const
        {
            switch (this->Status)
            {
                case PageDescriptorStatus::Free:
                    return "Free";
                case PageDescriptorStatus::InUse:
                    return "In Use";
                case PageDescriptorStatus::Caching:
                    return "Caching";
                case PageDescriptorStatus::Reserved:
                    return "Reserved";

                default:
                    return "UNKNOWN";
            }
        }

        __forceinline const char GetStatusChar() const
        {
            switch (this->Status)
            {
                case PageDescriptorStatus::Free:
                    return 'F';
                case PageDescriptorStatus::InUse:
                    return 'U';
                case PageDescriptorStatus::Caching:
                    return 'C';
                case PageDescriptorStatus::Reserved:
                    return 'R';

                default:
                    return 'X';
            }
        }

        // __forceinline Terminals::TerminalWriteResult PrintToTerminal(Terminals::TerminalBase * const term)
        // {
        //     return term->WriteFormat("|%c|D@%Xp|R-%X4|A-%X2|I-%X4|"
        //         , this->GetStatusChar(), this
        //         , this->ReferenceCount.Load()
        //         , this->Accesses.Load()
        //         , (uint32_t)this->StackIndex);
        // }
#endif

    };

    /**
     * Manages a region of memory in which pages can be allocated.
     */
    class PageAllocationSpace
    {
        /*  TODO:
         *  - Maybe take care of page colouring?
         */

        /*  Inner workings:
         *      The page allocator maps a number of allocable pages.
         *      The free pages reside on a stack.
         *      (the control pages [containing the map and stack] are
         *      not mapped; they are implicitly reserved.)
         */

    public:

        /*  Statics  */

        static __forceinline psize_t GetControlPageCountOfRange(
              paddr_t const phys_start
            , paddr_t const phys_end
            , psize_t const page_size)
        {
            const psize_t len = phys_end - phys_start;

            return (len /  page_size                                            )
                 - (len / (page_size + sizeof(PageDescriptor) + sizeof(pgind_t)));
            //  Total page count minus allocable page count.
        }

        /*  Proeprties  */

#define PROP(type, name)                                     \
    private:                                                 \
        type name;                                           \
    public:                                                  \
        __forceinline type MCATS2(Get, name)() const \
        {                                                    \
            return this->name;                               \
        }
#define CNST(type, name)                                     \
    public:                                                  \
        type const name;                                     \
        __forceinline type MCATS2(Get, name)() const \
        {                                                    \
            return this->name;                               \
        }

        CNST(paddr_t, MemoryStart)          //  Start of the allocation space.
        CNST(paddr_t, MemoryEnd)            //  End of the allocation space.
        PROP(paddr_t, AllocationStart)      //  Start of space which can be freely allocated.
        CNST(paddr_t, AllocationEnd)        //  End of space which can be freely allocated.

        CNST(psize_t, PageSize)             //  Size of a memory page.

        CNST(psize_t, PageCount)            //  Total number of pages in the allocation space.
        CNST(psize_t, Size)                 //  Total number of bytes in the allocation space.
        CNST(psize_t, AllocablePageCount)   //  Total number of pages which can be allocated.
        PROP(psize_t, AllocableSize)        //  Total number of bytes which can be allocated.

        PROP(psize_t, ControlPageCount)     //  Number of pages used for control structures (descriptor map and stacks).
        //PROP(psize_t, MapSize)              //  Number of bytes used for descriptor map.
        //PROP(psize_t, StackSize)            //  Number of bytes used for the page stack.

        PROP(psize_t, StackFreeTop)         //  Top of the free page stack.
        //PROP(psize_t, StackCacheTop)        //  Top of the cache page stack.

        PROP(psize_t, FreePageCount)        //  Number of unallocated pages.
        PROP(psize_t, FreeSize)             //  Number of bytes in unallocated pages.
        PROP(psize_t, ReservedPageCount)    //  Number of reserved pages.
        PROP(psize_t, ReservedSize)         //  Number of bytes in reserved pages.

    public:

        /*  Constructors    */

        PageAllocationSpace();
        PageAllocationSpace(paddr_t const phys_start, paddr_t const phys_end
                                  , psize_t const page_size);

        __cold Handle InitializeControlStructures();

        PageAllocationSpace(PageAllocationSpace const &) = delete;
        PageAllocationSpace & operator =(PageAllocationSpace const &) = delete;

        /*  Page manipulation  */

        __cold Handle ReservePageRange(pgind_t const start, psize_t const count, PageReservationOptions const options);
        inline Handle ReservePageRange(pgind_t const start, psize_t const count)
        {
            return this->ReservePageRange(start, count, PageReservationOptions::OnlyFree);
        }

        inline Handle ReserveByteRange(paddr_t const phys_start, psize_t const length, PageReservationOptions const options)
        {
            return this->ReservePageRange((phys_start - this->AllocationStart) / this->PageSize, length / this->PageSize, options);
        }
        inline Handle ReserveByteRange(paddr_t const phys_start, psize_t const length)
        {
            return this->ReserveByteRange(phys_start, length, PageReservationOptions::OnlyFree);
        }

        __hot Handle FreePageRange(pgind_t const start, psize_t const count);
        inline Handle FreeByteRange(paddr_t const phys_start, psize_t const length)
        {
            return this->FreePageRange((phys_start - this->AllocationStart) / this->PageSize, length / this->PageSize);
        }
        inline Handle FreePageAtAddress(paddr_t const phys_addr)
        {
            return this->FreePageRange((phys_addr - this->AllocationStart) / this->PageSize, 1);
        }

        __hot paddr_t AllocatePage(PageDescriptor * & desc);
        __hot paddr_t AllocatePages(psize_t const count);

        inline bool ContainsRange(paddr_t const phys_start, psize_t const length) const
        {
            return ( phys_start           >= this->AllocationStart)
                && ((phys_start + length) <= this->AllocationEnd);
        }

        /*  Miscellaneous  */

        __cold inline void RemapControlStructures(vaddr_t const newAddr)
        {
            withLock (this->Locker)
            {
                this->Stack = (pgind_t *)((vaddr_t)this->Stack - (vaddr_t)this->Map + newAddr);
                this->Map = (PageDescriptor *)newAddr;
            }
        }

        /**
         *  Tries to get the descriptor of the page at the given physical address.
         */
        __forceinline bool TryGetPageDescriptor(paddr_t const paddr, PageDescriptor * & res)
        {
            if (paddr < this->AllocationStart || paddr >= this->AllocationEnd)
                return false;

            res = this->Map + ((paddr - this->AllocationStart) / this->PageSize);

            return true;
        }

    private:

        /*  Utilitary Methods  */

        //  Pops a page off the stack. (stack selected based on status)
        __hot Handle PopPage(pgind_t const ind);

        /*  Fields  */

        PageDescriptor * Map;
        //  Pointers to the allocation map within the space.
        pgind_t * Stack;
        //  El stacko de páginas libres. Lmao.

        Synchronization::SpinlockUninterruptible<> Locker;

    public:

        PageAllocationSpace * Next;
        PageAllocationSpace * Previous;

        /*  Debug  */

#ifdef __BEELZEBUB__DEBUG
        // __cold Terminals::TerminalWriteResult PrintStackToTerminal(Terminals::TerminalBase * const term, bool const details);
#endif

    };// __packed;

    /**
     *  Manages allocation of memory pages using a linked list of
     *  page allocation spaces.
     */
    class PageAllocator
    {
    public:

        /*  Constructors  */

        PageAllocator();
        PageAllocator(PageAllocationSpace * const first);

        PageAllocator(PageAllocator const &) = delete;
        PageAllocator & operator =(PageAllocator const &) = delete;

        /*  Page Manipulation  */

        __cold Handle ReserveByteRange(paddr_t const phys_start, psize_t const length, PageReservationOptions const options);
        __forceinline Handle ReserveByteRange(paddr_t const phys_start, psize_t const length)
        {
            return this->ReserveByteRange(phys_start, length, PageReservationOptions::OnlyFree);
        }

        __hot Handle FreeByteRange(paddr_t const phys_start, psize_t const length);
        Handle FreePageAtAddress(paddr_t const phys_addr);

        __hot paddr_t AllocatePage(PageAllocationOptions const options, PageDescriptor * & desc);
        __forceinline paddr_t AllocatePage(PageDescriptor * & desc)
        {
            return this->AllocatePage(PageAllocationOptions::GeneralPages, desc);
        }

        __hot paddr_t AllocatePages(psize_t const count, PageAllocationOptions const options);

        PageAllocationSpace * GetSpaceContainingAddress(paddr_t const address);
        bool ContainsRange(paddr_t const phys_start, psize_t const length);

        __hot bool TryGetPageDescriptor(paddr_t const paddr, PageDescriptor * & res);

        /*  Synchronization  */

        //  Used for mutual exclusion over the linking pointers of the
        //  allocation spaces.
        Synchronization::SpinlockUninterruptible<> ChainLock;

        /*  Space Chaining  */

        PageAllocationSpace * FirstSpace;
        PageAllocationSpace * LastSpace;

        __cold void PreppendAllocationSpace(PageAllocationSpace * const space);
        __cold void AppendAllocationSpace(PageAllocationSpace * const space);

        __cold void RemapLinks(vaddr_t const oldAddr, vaddr_t const newAddr);
    };
}}
