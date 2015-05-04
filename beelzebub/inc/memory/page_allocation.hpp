#pragma once

#include <kernel.hpp>
#include <terminals/base.hpp>
#include <arc/synchronization/spinlock.hpp>
#include <handles.h>
#include <metaprogramming.h>

using namespace Beelzebub::Terminals;
using namespace Beelzebub::Synchronization;

namespace Beelzebub { namespace Memory
{
    /**
     * Represents possible statuses of a memory page.
     */
    enum class PageStatus : uint16_t
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
        /*  Fields  */

        //  Index of the page in the allocation stack.
        uint64_t StackIndex;

        //  Page status
        PageStatus Status;
        //  Access count
        uint16_t Accesses;

        //  Number of references to this page.
        uint32_t ReferenceCount;

        //  Device (file?) from which the page comes.
        Handle Source;
        //  File page descriptor (TODO - in the really long term)
        void * FilePageDescriptor;

        /*  Constructors  */

        PageDescriptor() = default;
        PageDescriptor(PageDescriptor const&) = default;

        __bland __forceinline PageDescriptor(const uint64_t stackIndex)
            : StackIndex( stackIndex )
            , Status(PageStatus::Free)
            , Accesses(0)
            , ReferenceCount(0)
            , Source()
            , FilePageDescriptor(0ULL)
        {

        }

        __bland __forceinline PageDescriptor(const uint64_t stackIndex
                                           , const PageStatus status)
            : StackIndex( stackIndex )
            , Status(status)
            , Accesses(0)
            , ReferenceCount(0)
            , Source()
            , FilePageDescriptor(0ULL)
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

            if (this->Accesses == (uint16_t)0xFFFF)
                return this->Accesses = 1;
            else
                return ++this->Accesses;
        }

        /*  Reference count  */

        __bland __forceinline void ResetReferenceCount()
        {
            this->ReferenceCount = 0;
        }

        __bland __forceinline uint32_t IncrementReferenceCount()
        {
            //  TODO: Handle overflow..?
            
            return ++this->ReferenceCount;
        }

        __bland __forceinline uint32_t DecrementReferenceCount()
        {
            //  TODO: Handle underflow..?
            
            return --this->ReferenceCount;
        }

        /*  Status  */

        __bland __forceinline void Free()
        {
            this->Status = PageStatus::Free;

            this->ResetAccesses();
            this->ResetReferenceCount();
        }

        __bland __forceinline void Use()
        {
            this->Status = PageStatus::InUse;

            this->ResetAccesses();
            this->ResetReferenceCount();
        }

        __bland __forceinline void Reserve()
        {
            this->Status = PageStatus::Reserved;

            this->ResetReferenceCount();
        }

        /*  Debug  */

#ifdef __BEELZEBUB__DEBUG
        __bland __forceinline const char * GetStatusString() const
        {
            switch (this->Status)
            {
                case PageStatus::Free:
                    return "Free";
                case PageStatus::InUse:
                    return "In Use";
                case PageStatus::Caching:
                    return "Caching";
                case PageStatus::Reserved:
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

    } __attribute__((packed));

    /**
     * Describes a region of memory in which pages can be allocated.
     */
    class PageAllocationSpace
    {
        /*  TODO:
         *  - Mutual exclusion: - Maybe over the whole allocator,
         *                        but this would shuck.
         *                      - Maybe over ranges over every N pages,
         *                        but this may complicate the code.
         *  - Maybe take care of page colouring?
         */

        /*  Inner workings:
         *      The page allocator maps a number of allocable pages.
         *      The free pages reside on a stack.
         *      (the control pages [containing the map and stack] are
         *      not mapped; they are implicitly reserved.)
         */

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
        const type name;                                     

        PROP(paddr_t, MemoryStart)          //  Start of the allocation space.
        PROP(paddr_t, MemoryEnd)            //  End of the allocation space.
        PROP(paddr_t, AllocationStart)      //  Start of space which can be freely allocated.
        PROP(paddr_t, AllocationEnd)        //  End of space which can be freely allocated.

        PROP(psize_t, PageSize)             //  Size of a memory page.

        PROP(psize_t, PageCount)            //  Total number of pages in the allocation space.
        PROP(psize_t, Size)                 //  Total number of bytes in the allocation space.
        PROP(psize_t, AllocablePageCount)   //  Total number of pages which can be allocated.
        PROP(psize_t, AllocableSize)        //  Total number of bytes which can be allocated.

        PROP(psize_t, ControlPageCount)     //  Number of pages used for control structures (descriptor map and stacks).
        PROP(psize_t, MapSize)              //  Number of bytes used for descriptor map.
        PROP(psize_t, StackSize)            //  Number of bytes used for the page stack.

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
        
        PageAllocationSpace(PageAllocationSpace const&) = delete;

        /*  Page manipulation  */

        __bland Handle ReservePageRange(const pgind_t start, const psize_t count, const bool onlyFree);
        __bland __forceinline Handle ReservePageRange(const pgind_t start, const psize_t count)
        {
            return this->ReservePageRange(start, count, true);
        }

        __bland __forceinline Handle ReserveByteRange(const paddr_t phys_start, const psize_t length, const bool onlyFree)
        {
            return this->ReservePageRange((phys_start - this->MemoryStart) / this->PageSize, length / this->PageSize, onlyFree);
        }
        __bland __forceinline Handle ReserveByteRange(const paddr_t phys_start, const psize_t length)
        {
            return this->ReserveByteRange(phys_start, length, true);
        }

        __bland Handle FreePageRange(const pgind_t start, const psize_t count);

        __bland paddr_t AllocatePage();
        __bland paddr_t AllocatePages(const psize_t count);

        /*  Synchronization  */

        __bland __forceinline void Lock()
        {
            (&this->Locker)->Acquire();
        }

        __bland __forceinline void Unlock()
        {
            (&this->Locker)->Release();
        }

    private:

        /*  Fields  */

        PageDescriptor * Map;
        //  Pointers to the allocation map within the space.
        pgind_t * Stack;
        //  El stacko de páginas libres. Lmao.

        Spinlock Locker;

    public:

        //PageAllocationSpace * Next;

        /*  Debug  */

#ifdef __BEELZEBUB__DEBUG
        __bland TerminalWriteResult PrintStackToTerminal(TerminalBase * const term, const bool details);
#endif

    };// __attribute__((packed));
}}
