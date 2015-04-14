#pragma once

#include <metaprogramming.h>

__extern __forceinline __bland u64 RoundUp(u64 value, u64 step) __attribute__((const));
__extern __forceinline __bland u64 RoundDown(u64 value, u64 step) __attribute__((const));
__extern __forceinline __bland u64 RoundUpDiff(u64 value, u64 step) __attribute__((const));

u64 RoundUp(u64 value, u64 step)
{
	return value + ((step - (value % step)) % step);
}

u64 RoundDown(u64 value, u64 step)
{
	return value - (value & step);
}

u64 RoundUpDiff(u64 value, u64 step)
{
	return (step - (value % step)) % step;
}
