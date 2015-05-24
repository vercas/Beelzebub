/*  THE DECLARATIONS IN THIS FILE ARE DEFINED IN ARCHITECTURE-SPECIFIC
 *  FILES
 */

#pragma once

//#include <memory/page_allocator.hpp>
#include <handles.h>
#include <metaprogramming.h>

namespace Beelzebub { namespace Memory
{
    /**
     * Represents types of pages that can be allocated.
     */
    enum class PageFlags
    {
        //  Shared by all processes.
        Global     = 0x01,
        //  Accessible by user code.
        Userland   = 0x02,
        //  Writing to the page is allowed.
        Writable   = 0x04,
        //  Executing code from the page is allowed.
        Executable = 0x08,
    };

    //  Bitwise OR.
    inline PageFlags operator |(PageFlags a, PageFlags b)
    { return static_cast<PageFlags>(static_cast<int>(a) | static_cast<int>(b)); }

    //  Bitwise AND.
    inline PageFlags operator &(PageFlags a, PageFlags b)
    { return static_cast<PageFlags>(static_cast<int>(a) & static_cast<int>(b)); }

    //  Equality.
    inline bool operator ==(int a, PageFlags b)
    { return a == static_cast<int>(b); }

    //  Inequality.
    inline bool operator !=(int a, PageFlags b)
    { return a != static_cast<int>(b); }

    /**
     * Represents types of pages that can be allocated.
     */
    enum class AllocatedPageType : uint32_t
    {
        //  The physical page selected will be suitable for general use
        //  in the kernel heap and applications. 64-bit preferred.
        PhysicalGeneral    = 0x00000000,
        //  The physical page selected will have a 32-bit address,
        //  suitable for certain devices.
        Physical32bit      = 0x00000010,

        //  The virtual page will be located in areas specific to the kernel heap.
        VirtualKernelHeap  = 0x00000000,
        //  The virtual page will be located in userland-specific areas.
        VirtualUser        = 0x00080000,
        //  The virtual page will be located in 32-bit memory locations,
        //  suitable for 32-bit applications and kernel modules.
        Virtual32bit       = 0x00100000,
    };

    //  Bitwise OR.
    inline AllocatedPageType operator |(AllocatedPageType a, AllocatedPageType b)
    { return static_cast<AllocatedPageType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }

    //  Bitwise AND.
    inline AllocatedPageType operator &(AllocatedPageType a, AllocatedPageType b)
    { return static_cast<AllocatedPageType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); }

    //  Equality.
    inline bool operator ==(uint32_t a, AllocatedPageType b)
    { return a == static_cast<uint32_t>(b); }

    //  Inequality.
    inline bool operator !=(uint32_t a, AllocatedPageType b)
    { return a != static_cast<uint32_t>(b); }

    /**
     * Describes a region of memory in which pages can be allocated.
     */
    class MemoryAllocator
    {

    };
}}
