#pragma once

#include <memory/page_allocation.hpp>
#include <handles.h>
#include <metaprogramming.h>

namespace Beelzebub { namespace Memory
{
	/*	Initializes the memory based on the given ranges.	*/
	__bland Handle Initialize(const PageAllocationSpace * const ranges, const size_t count);
}}
