#pragma once

#include <memory/manager.hpp>
#include <memory/virtual_allocator.hpp>
#include <synchronization/spinlock.hpp>
#include <handles.h>
#include <metaprogramming.h>

namespace Beelzebub { namespace Memory
{
    /**
     *  A memory manager for AMD64.
     */
    class MemoryManagerAmd64 : MemoryManager
    {
    public:

        /*  Components  */

        VirtualAllocationSpace * Vas;

        /*  Locks  */

        static Spinlock KernelLock;
        Spinlock UserLock;
    };
}}
