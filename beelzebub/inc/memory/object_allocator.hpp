#pragma once

#if   defined(__BEELZEBUB__TEST_OBJA)
    #define private public
    #define protected public
#endif

#include <memory/object_allocator_pools.hpp>

#include <synchronization/atomic.hpp>
#include <handles.h>

#if   defined(__BEELZEBUB_KERNEL)
    #include <synchronization/spinlock_uninterruptible.hpp>
#endif

#if defined(__BEELZEBUB_KERNEL)
    #define OBJA_LOCK_TYPE SpinlockUninterruptible<>

    #define OBJA_COOK_TYPE int_cookie_t
#else
    //  To do: userland will require a mutex here.
#endif

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;

namespace Beelzebub { namespace Memory
{
    /*  First, the normal SMP-aware object allocator.  */
    #define OBJA_POOL_TYPE      ObjectPool
    #define OBJA_ALOC_TYPE      ObjectAllocator
    #define OBJA_MULTICONSUMER  true
    #include <memory/object_allocator_hbase.inc>
    #undef OBJA_MULTICONSUMER
    #undef OBJA_ALOC_TYPE
    #undef OBJA_POOL_TYPE
}}

#undef OBJA_COOK_TYPE
#undef OBJA_LOCK_TYPE

#if   defined(__BEELZEBUB__TEST_OBJA)
    #undef private
    #undef protected
#endif
