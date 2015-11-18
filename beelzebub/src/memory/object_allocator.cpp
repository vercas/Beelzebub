/**
 *  The object allocator uses a bold strategy to achieve multiprocessing safety.
 *  
 *  First of all, there's the object pool.
 *  All of its properties are going to be read and changed while it's locked.
 *  This includes linkage, counters and free object indexes.
 *  When a method tries to work with a pool
 *  
 *  Then, there's the 
 */

#include <memory/object_allocator.hpp>

#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;

#if defined(__BEELZEBUB_KERNEL)
    #define OBJA_LOCK_TYPE SpinlockUninterruptible<>

    #define OBJA_COOK_TYPE int_cookie_t
#else
    //  To do: userland will require a mutex here.
#endif

#define OBJA_POOL_TYPE      ObjectPool
#define OBJA_ALOC_TYPE      ObjectAllocator
#define OBJA_MULTICONSUMER  true
#include <memory/object_allocator_cbase.inc>
#undef OBJA_MULTICONSUMER
#undef OBJA_ALOC_TYPE
#undef OBJA_POOL_TYPE

#undef OBJA_COOK_TYPE
#undef OBJA_LOCK_TYPE
