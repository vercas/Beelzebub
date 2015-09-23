#pragma once

#include <metaprogramming.h>

#ifdef __cplusplus

#ifdef __BEELZEBUB__ARCH_X86
template<typename TNum1, typename TNum2>
__bland __forceinline __const constexpr auto RoundUp(TNum1 value, TNum2 step) -> decltype(((value + step - 1) / step) * step)
{
    return ((value + step - 1) / step) * step;
}
#else
template<typename TNum1, typename TNum2>
__bland __forceinline __const constexpr auto RoundUp(TNum1 value, TNum2 step) -> decltype(value + ((step - (value % step)) % step))
{
    return value + ((step - (value % step)) % step);
}
#endif

template<typename TNum1, typename TNum2>
__bland __forceinline __const constexpr auto RoundDown(TNum1 value, TNum2 step) -> decltype(value - (value & step))
{
    return value - (value & step);
}

template<typename TNum1, typename TNum2>
__bland __forceinline __const constexpr auto RoundUpDiff(TNum1 value, TNum2 step) -> decltype((step - (value % step)) % step)
{
    return (step - (value % step)) % step;
}

#else

#ifdef __BEELZEBUB__ARCH_X86
__bland __forceinline constexpr uint64_t RoundUp64(uint64_t value, uint64_t step) __const
{
    return ((value + step - 1) / step) * step;
}
__bland __forceinline constexpr uint32_t RoundUp32(uint32_t value, uint32_t step) __const
{
    return ((value + step - 1) / step) * step;
}
#else
__bland __forceinline constexpr uint64_t RoundUp64(uint64_t value, uint64_t step) __const
{
    return value + ((step - (value % step)) % step);
}
__bland __forceinline constexpr uint32_t RoundUp32(uint32_t value, uint32_t step) __const
{
    return value + ((step - (value % step)) % step);
}
#endif

__bland __forceinline constexpr uint64_t RoundDown64(uint64_t value, uint64_t step) __const
{
    return value - (value & step);
}
__bland __forceinline constexpr uint32_t RoundDown32(uint32_t value, uint32_t step) __const
{
    return value - (value & step);
}

__bland __forceinline constexpr uint64_t RoundUpDiff64(uint64_t value, uint64_t step) __const
{
    return (step - (value % step)) % step;
}
__bland __forceinline constexpr uint32_t RoundUpDiff32(uint32_t value, uint32_t step) __const
{
    return (step - (value % step)) % step;
}

#endif
