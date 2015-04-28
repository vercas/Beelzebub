#pragma once

#include <handles.h>
#include <metaprogramming.h>

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

        /*  Constructors    */

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
            return ++this->ReferenceCount;
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

        /*  Proeprties  */

#define PROP(type, name)                               \
    private:                                           \
        type name;                                     \
    public:                                            \
        __bland __forceinline type MCATS2(Get, name)() \
        {                                              \
            return this->name;                         \
        }

        PROP(uintptr_t, MemoryStart)        //  Start of the allocation space.
        PROP(uintptr_t, MemoryEnd)          //  End of the allocation space.
        PROP(uintptr_t, AllocationStart)    //  Start of space which can be freely allocated.
        PROP(uintptr_t, AllocationEnd)      //  End of space which can be freely allocated.

        PROP(size_t, PageSize)              //  Size of a memory page.

        PROP(size_t, PageCount)             //  Total number of pages in the allocation space.
        PROP(size_t, Size)                  //  Total number of bytes in the allocation space.
        PROP(size_t, AllocablePageCount)    //  Total number of pages which can be allocated.
        PROP(size_t, AllocableSize)         //  Total number of bytes which can be allocated.
        
        PROP(size_t, ControlPageCount)      //  Number of pages used for control structures (descriptor map and stacks).
        PROP(size_t, MapSize)               //  Number of bytes used for descriptor map.
        PROP(size_t, StackSize)             //  Number of bytes used for the page stack.

        PROP(size_t, StackFreeTop)          //  Top of the free page stack.
        PROP(size_t, StackCacheTop)         //  Top of the cache page stack.

        PROP(size_t, FreePageCount)         //  Number of unallocated pages.
        PROP(size_t, FreeSize)              //  Number of bytes in unallocated pages.
        PROP(size_t, ReservedPageCount)     //  Number of reserved pages.
        PROP(size_t, ReservedSize)          //  Number of bytes in reserved pages.

    public:

        /*  Constructors    */

        __bland PageAllocationSpace();
        __bland PageAllocationSpace(const uintptr_t phys_start, const uintptr_t phys_end
                                  , const size_t page_size);

        /*  Page manipulation  */

        __bland Handle ReservePageRange(const size_t start, const size_t count);
        __bland Handle ReserveByteRange(const uintptr_t phys_start, const size_t length);

        __bland void * AllocatePages(const size_t count);
        __bland Handle FreePages(const void * const phys_start, const size_t count);

    private:

        /*  Fields  */

        PageDescriptor * Map, * MapEnd;
        //  Pointers to the allocation map within the space.
        size_t * Stack;
        //  El stacko de páginas libres. Lmao.
        size_t CurrentIndex;
        //  Used for round-robin checkin'.

        //  Current size: 144 bytes. :(

    public:

        //PageAllocationSpace * Next;

    } __attribute__((packed));
}}
