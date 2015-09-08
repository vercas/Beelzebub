#pragma once

#include <metaprogramming.h>

__extern __forceinline __bland uint64_t RoundUp(uint64_t value, uint64_t step) __const;
__extern __forceinline __bland uint64_t RoundDown(uint64_t value, uint64_t step) __const;
__extern __forceinline __bland uint64_t RoundUpDiff(uint64_t value, uint64_t step) __const;

uint64_t RoundUp(uint64_t value, uint64_t step)
{
	return value + ((step - (value % step)) % step);
}

uint64_t RoundDown(uint64_t value, uint64_t step)
{
	return value - (value & step);
}

uint64_t RoundUpDiff(uint64_t value, uint64_t step)
{
	return (step - (value % step)) % step;
}
