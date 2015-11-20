#pragma once

#include <memory/object_allocator_pools.hpp>

namespace Beelzebub { namespace Memory
{
    __bland Handle AcquirePoolInKernelHeap(size_t objectSize
                                         , size_t headerSize
                                         , size_t minimumObjects
                                         , ObjectPoolBase * & result);

    __bland Handle EnlargePoolInKernelHeap(size_t objectSize
                                         , size_t headerSize
                                         , size_t minimumExtraObjects
                                         , ObjectPoolBase * pool);

    __bland Handle ReleasePoolFromKernelHeap(size_t objectSize
                                           , size_t headerSize
                                           , ObjectPoolBase * pool);
}}
