#pragma once

#include <synchronization/atomic.hpp>
#include <handles.h>

extern Beelzebub::Synchronization::Atomic<size_t> ObjectAllocatorTestJunction1;
extern Beelzebub::Synchronization::Atomic<size_t> ObjectAllocatorTestJunction2;
extern Beelzebub::Synchronization::Atomic<size_t> ObjectAllocatorTestJunction3;

__cold __bland Beelzebub::Handle TestObjectAllocator(bool const bsp);
