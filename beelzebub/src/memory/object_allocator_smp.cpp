#if   defined(__BEELZEBUB_SETTINGS_SMP)
//  Since this file only implements stuff, there's nothing to do without SMP
//  support.

    #include <memory/object_allocator_smp.hpp>

    #include <math.h>
    #include <debug.hpp>

    using namespace Beelzebub;
    using namespace Beelzebub::Memory;

    #if defined(__BEELZEBUB_KERNEL)
        #define OBJA_LOCK_TYPE Beelzebub::Synchronization::SpinlockUninterruptible<>

        #define OBJA_COOK_TYPE int_cookie_t
    #else
        //  To do: userland will require a mutex here.
    #endif

    #define OBJA_POOL_TYPE      ObjectPoolSmp
    #define OBJA_ALOC_TYPE      ObjectAllocatorSmp
    #define OBJA_MULTICONSUMER  true
    #include <memory/object_allocator_cbase.inc>
    #undef OBJA_MULTICONSUMER
    #undef OBJA_ALOC_TYPE
    #undef OBJA_POOL_TYPE

    #undef OBJA_COOK_TYPE
    #undef OBJA_LOCK_TYPE

#endif
