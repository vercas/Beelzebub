#if   defined(__BEELZEBUB__TEST_OBJA)
    #define private public
    #define protected public
#endif

#include <memory/object_allocator_pools.hpp>

namespace Beelzebub { namespace Memory
{
    /*  First, the normal SMP-aware object allocator.  */
    #define OBJA_ALOC_TYPE      ObjectAllocator
    #include <memory/object_allocator_hbase.inc>
    #undef OBJA_ALOC_TYPE
    #undef OBJA_POOL_TYPE   //  NEEDS TO BE UNDEFINED ANYWAY
}}

#if   defined(__BEELZEBUB__TEST_OBJA)
    #undef private
    #undef protected
#endif
