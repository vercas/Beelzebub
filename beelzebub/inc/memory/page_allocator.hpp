#pragma once

#include <metaprogramming.h>
#include <debug.hpp>
#include <kernel.hpp>
#include <terminals/base.hpp>
#include <synchronization/spinlock.hpp>
#include <handles.h>

using namespace Beelzebub::Terminals;
using namespace Beelzebub::Synchronization;

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
        static const uint16_t MaxAccesses = (uint16_t)0xFFFF;

        /*  Fields  */

        //  Device (file?) from which the page comes.
        Handle Source;
        //  File page descriptor (TODO - in the really long term)
        void * FilePageDescriptor;

        //  Index of the page in the allocation stack.
        pgind_t StackIndex;

        //  Page status
        PageDescriptorStatus Status;
        //  Access count
        uint16_t Accesses;

        //  Number of references to this page.
        volatile uint32_t ReferenceCount;

        /*  Constructors  */

        PageDescriptor() = default;
        PageDescriptor(PageDescriptor const&) = default;

        __bland __forceinline PageDescriptor(const uint64_t stackIndex)
            : Source()
            , FilePageDescriptor(0ULL)
            , StackIndex( stackIndex )
            , Status(PageDescriptorStatus::Free)
            , Accesses(0)
            , ReferenceCount(0)
        {

        }

        __bland __forceinline PageDescriptor(const uint64_t stackIndex
                                           , const PageDescriptorStatus status)
            : Source()
            , FilePageDescriptor(0ULL)
            , StackIndex( stackIndex )
            , Status(status)
            , Accesses(0)
            , ReferenceCount(0)
        {

        }

        /*  Accesses  */

        __bland __forceinline void ResetAccesses()
        {
            this->Accesses = 0;
        }

        __bland __forceinline uint16_t IncrementAccesses()
        {
            //  An exact count isn't really required, but it may
            //  prove useful.

            uint16_t ret = __sync_add_and_fetch(&this->Accesses, 1);

            if unlikely(ret == 0)
            {
                __sync_val_compare_and_swap(&this->Accesses, 0, 1);

                return 1;
            }
            else
                return ret;
        }

        /*  Reference count  */

        __bland __forceinline void ResetReferenceCount()
        {
            this->ReferenceCount = 0;
            //  This really should be atomic...
        }

        __bland __forceinline uint32_t IncrementReferenceCount()
        {
            return __sync_add_and_fetch(&this->ReferenceCount, 1);
        }

        __bland __forceinline uint32_t DecrementReferenceCount()
        {
            assert(this->ReferenceCount > 0,
                "Attempting to decrement reference count of a page count 0!");

            return __sync_sub_and_fetch(&this->ReferenceCount, 1);
        }

        /*  Status  */

        __bland __forceinline void Free()
        {
            this->Status = PageDescriptorStatus::Free;

            this->ResetAccesses();
            this->ResetReferenceCount();
        }

        __bland __forceinline void Use()
        {
            this->Status = PageDescriptorStatus::InUse;

            this->ResetAccesses();
            this->ResetReferenceCount();
        }

        __bland __forceinline void Reserve()
        {
            this->Status = PageDescriptorStatus::Reserved;

            this->ResetReferenceCount();
        }

        /*  Debug  */

#ifdef __BEELZEBUB__DEBUG
        __bland __forceinline const char * GetStatusString() const
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

        __bland __forceinline TerminalWriteResult PrintToTerminal(TerminalBase * const term)
        {
            return term->WriteFormat("");
        }
#endif

    } __packed;

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

        static __bland __forceinline psize_t GetControlPageCountOfRange(
              const paddr_t phys_start
            , const paddr_t phys_end
            , const psize_t page_size)
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
        __bland __forceinline type MCATS2(Get, name)() const \
        {                                                    \
            return this->name;                               \
        }
#define CNST(type, name)                                     \
    public:                                                  \
        const type name;                                     \
        __bland __forceinline type MCATS2(Get, name)() const \
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
        PROP(psize_t, StackCacheTop)        //  Top of the cache page stack.

        PROP(psize_t, FreePageCount)        //  Number of unallocated pages.
        PROP(psize_t, FreeSize)             //  Number of bytes in unallocated pages.
        PROP(psize_t, ReservedPageCount)    //  Number of reserved pages.
        PROP(psize_t, ReservedSize)         //  Number of bytes in reserved pages.

    public:

        /*  Constructors    */

        __bland PageAllocationSpace();
        __bland PageAllocationSpace(const paddr_t phys_start, const paddr_t phys_end
                                  , const psize_t page_size);

        __cold __bland Handle InitializeControlStructures();

        PageAllocationSpace(PageAllocationSpace const &) = delete;
        PageAllocationSpace & operator =(const PageAllocationSpace &) = delete;

        /*  Page manipulation  */

        __cold __bland Handle ReservePageRange(const pgind_t start, const psize_t count, const PageReservationOptions options);
        __bland __forceinline Handle ReservePageRange(const pgind_t start, const psize_t count)
        {
            return this->ReservePageRange(start, count, PageReservationOptions::OnlyFree);
        }

        __bland __forceinline Handle ReserveByteRange(const paddr_t phys_start, const psize_t length, const PageReservationOptions options)
        {
            return this->ReservePageRange((phys_start - this->AllocationStart) / this->PageSize, length / this->PageSize, options);
        }
        __bland __forceinline Handle ReserveByteRange(const paddr_t phys_start, const psize_t length)
        {
            return this->ReserveByteRange(phys_start, length, PageReservationOptions::OnlyFree);
        }

        __hot __bland Handle FreePageRange(const pgind_t start, const psize_t count);
        __bland __forceinline Handle FreeByteRange(const paddr_t phys_start, const psize_t length)
        {
            return this->FreePageRange((phys_start - this->MemoryStart) / this->PageSize, length / this->PageSize);
        }
        __bland __forceinline Handle FreePageAtAddress(const paddr_t phys_addr)
        {
            return this->FreePageRange((phys_addr - this->MemoryStart) / this->PageSize, 1);
        }

        __hot __bland paddr_t AllocatePage(PageDescriptor * & desc);
        __hot __bland paddr_t AllocatePages(const psize_t count);

        __bland __forceinline bool ContainsRange(const paddr_t phys_start, const psize_t length) const
        {
            return ( phys_start           >= this->AllocationStart)
                && ((phys_start + length) <= this->AllocationEnd);
        }

        /*  Synchronization  */

        __bland __forceinline void Lock()
        {
            (&this->Locker)->Acquire();
        }

        __bland __forceinline void Unlock()
        {
            (&this->Locker)->Release();
        }

        /*  Miscellaneous  */

        __cold __bland __forceinline void RemapControlStructures(const vaddr_t newAddr)
        {
            this->Lock();

            this->Stack = (pgind_t *)((vaddr_t)this->Stack - (vaddr_t)this->Map + newAddr);
            this->Map = (PageDescriptor *)newAddr;

            this->Unlock();
        }

        /**
         *  Tries to get the descriptor of the page at the given physical address.
         */
        __bland __forceinline bool TryGetPageDescriptor(const paddr_t paddr, PageDescriptor * & res)
        {
            if (paddr < this->AllocationStart || paddr >= this->AllocationEnd)
                return false;

            res = this->Map + ((paddr - this->AllocationStart) / this->PageSize);

            return true;
        }

    private:

        /*  Utilitary Methods  */

        //  Pops a page off the stack. (stack selected based on status)
        __hot __bland Handle PopPage(const pgind_t ind);

        /*  Fields  */

        PageDescriptor * Map;
        //  Pointers to the allocation map within the space.
        pgind_t * Stack;
        //  El stacko de páginas libres. Lmao.

        Spinlock Locker;

    public:

        PageAllocationSpace * Next;
        PageAllocationSpace * Previous;

        /*  Debug  */

#ifdef __BEELZEBUB__DEBUG
        __cold __bland TerminalWriteResult PrintStackToTerminal(TerminalBase * const term, const bool details);
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

        __bland PageAllocator();
        __bland PageAllocator(PageAllocationSpace * const first);

        PageAllocator(PageAllocator const &) = delete;
        PageAllocator & operator =(const PageAllocator &) = delete;

        /*  Page Manipulation  */

        __cold __bland Handle ReserveByteRange(const paddr_t phys_start, const psize_t length, const PageReservationOptions options);
        __bland __forceinline Handle ReserveByteRange(const paddr_t phys_start, const psize_t length)
        {
            return this->ReserveByteRange(phys_start, length, PageReservationOptions::OnlyFree);
        }

        __hot __bland Handle FreeByteRange(const paddr_t phys_start, const psize_t length);
        __bland Handle FreePageAtAddress(const paddr_t phys_addr);

        __hot __bland paddr_t AllocatePage(const PageAllocationOptions options, PageDescriptor * & desc);
        __bland __forceinline paddr_t AllocatePage(PageDescriptor * & desc)
        {
            return this->AllocatePage(PageAllocationOptions::GeneralPages, desc);
        }

        __hot __bland paddr_t AllocatePages(const psize_t count, const PageAllocationOptions options);

        __bland PageAllocationSpace * GetSpaceContainingAddress(const paddr_t address);
        __bland bool ContainsRange(const paddr_t phys_start, const psize_t length);

        __hot __bland bool TryGetPageDescriptor(const paddr_t paddr, PageDescriptor * & res);

        /*  Synchronization  */

        //  Used for mutual exclusion over the linking pointers of the
        //  allocation spaces.
        Spinlock ChainLock;

        __bland __forceinline void Lock()
        {
            (&this->ChainLock)->Acquire();
        }

        __bland __forceinline void Unlock()
        {
            (&this->ChainLock)->Release();
        }

        __bland __forceinline void Await()
        {
            (&this->ChainLock)->Await();
        }

        /*  Space Chaining  */

        PageAllocationSpace * FirstSpace;
        PageAllocationSpace * LastSpace;

        __cold __bland void PreppendAllocationSpace(PageAllocationSpace * const space);
        __cold __bland void AppendAllocationSpace(PageAllocationSpace * const space);

        __cold __bland void RemapLinks(const vaddr_t oldAddr, const vaddr_t newAddr);
    };
}}
