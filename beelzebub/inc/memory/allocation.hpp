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
    enum class AllocatedPageType : uint32_t
    {
        //  The physical page selected will be suitable for general use
        //  in the kernel heap and applications. 64-bit preferred.
        PhysicalGeneral    = 0x00000000,
        //  The physical page selected will have a 32-bit address,
        //  suitable for certain devices.
        Physical32bit      = 0x00000010,

        //  The virtual page will be located in kernel-specific areas.
        VirtualKernel      = 0x00000000,
        //  The virtual page will be located in userland-specific areas.
        VirtualUser        = 0x00010000,
        //  The virtual page will be located in 32-bit memory locations,
        //  suitable for 32-bit applications and kernel modules.
        Virtual32bit       = 0x00100000,
    };

    /**
     * Describes a region of memory in which pages can be allocated.
     */
    class MemoryAllocator
    {

    };
}}
