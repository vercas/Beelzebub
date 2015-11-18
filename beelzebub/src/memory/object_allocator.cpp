#include <memory/object_allocator.hpp>

#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;

#define OBJA_POOL_TYPE      ObjectPoolBase
#define OBJA_ALOC_TYPE      ObjectAllocator
#include <memory/object_allocator_cbase.inc>
#undef OBJA_ALOC_TYPE
#undef OBJA_POOL_TYPE
