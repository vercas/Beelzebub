#pragma once

#include <synchronization/smp_barrier.hpp>
#include <handles.h>

extern Beelzebub::Synchronization::SmpBarrier ObjectAllocatorTestBarrier1;
extern Beelzebub::Synchronization::SmpBarrier ObjectAllocatorTestBarrier2;
extern Beelzebub::Synchronization::SmpBarrier ObjectAllocatorTestBarrier3;

__cold __bland Beelzebub::Handle TestObjectAllocator(bool const bsp);
