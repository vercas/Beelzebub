/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#pragma once

#include <metaprogramming.h>

#ifdef __cplusplus
namespace Beelzebub
{

#ifdef __BEELZEBUB__ARCH_X86
template<typename TNum1, typename TNum2>
__forceinline __const constexpr auto RoundUp(const TNum1 value, const TNum2 step) -> decltype(((value + step - 1) / step) * step)
{
    return ((value + step - 1) / step) * step;
}
#else
template<typename TNum1, typename TNum2>
__forceinline __const constexpr auto RoundUp(const TNum1 value, const TNum2 step) -> decltype(value + ((step - (value % step)) % step))
{
    return value + ((step - (value % step)) % step);
}
#endif

template<typename TNum1, typename TNum2>
__forceinline __const constexpr auto RoundDown(const TNum1 value, const TNum2 step) -> decltype(value - (value & step))
{
    return value - (value % step);
}

template<typename TNum1, typename TNum2>
__forceinline __const constexpr auto RoundUpDiff(const TNum1 value, const TNum2 step) -> decltype((step - (value % step)) % step)
{
    return (step - (value % step)) % step;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"

template<typename TNum1>
__forceinline __const constexpr TNum1 Minimum(const TNum1 & a)
{
    return a;
}
template<typename TNum1, typename TNum2>
__forceinline __const constexpr auto Minimum(const TNum1 & a, const TNum2 & b) -> decltype((a < b) ? a : b)
{
    return (a < b) ? a : b;
}
template<typename TNum1, typename TNum2, typename TNum3>
__forceinline __const constexpr auto Minimum(const TNum1 & a, const TNum2 & b, const TNum3 & c) -> decltype((a < Minimum(b, c)) ? a : Minimum(b, c))
{
    auto _b = Minimum(b, c);

    return (a < _b) ? a : _b;
}

/*template<typename TNum1, typename ... TNumOthers>
__forceinline __const constexpr auto Minimum(const TNum1 & a, const TNumOthers & ... extras) -> decltype((a < Minimum(extras ...)) ? a : Minimum(extras ...))
{
    auto b = Minimum(extras ...);

    return (a < b) ? a : b;
}

template<typename TNum1, typename ... TNumOthers>
__forceinline __const constexpr auto Minimum(const TNum1 & a, const TNumOthers ... extras) -> decltype((a < Minimum(extras ...)) ? a : Minimum(extras ...))
{
    auto b = Minimum(extras ...);

    return (a < b) ? a : b;
}//*/

template<typename TNum1>
__forceinline __const constexpr TNum1 Maximum(const TNum1 & a)
{
    return a;
}
template<typename TNum1, typename TNum2>
__forceinline __const constexpr auto Maximum(const TNum1 & a, const TNum2 & b) -> decltype((a > b) ? a : b)
{
    return (a > b) ? a : b;
}
template<typename TNum1, typename TNum2, typename TNum3>
__forceinline __const constexpr auto Maximum(const TNum1 & a, const TNum2 & b, const TNum3 & c) -> decltype((a > Maximum(b, c)) ? a : Maximum(b, c))
{
    auto _b = Maximum(b, c);

    return (a > _b) ? a : _b;
}

#pragma GCC diagnostic pop

template<typename TNum>
__forceinline __const constexpr TNum GreatestCommonDivisor(const TNum a)
{
    return a;
}

template<typename TNum>
__forceinline __const constexpr TNum GreatestCommonDivisor(TNum a, TNum b)
{
    //  I hate homework.
    
    TNum t;

    while (b != 0)
    {
        t = b;
        b = a % b;
        a = t;
    }

    return a;
}

template<typename TNum>
__forceinline __const constexpr TNum GreatestCommonDivisor(TNum a, TNum b, const TNum c)
{
    b = GreatestCommonDivisor(b, c);

    TNum t;

    while (b != 0)
    {
        t = b;
        b = a % b;
        a = t;
    }

    return a;
}

#else

#ifdef __BEELZEBUB__ARCH_X86
__forceinline uint64_t RoundUp64(const uint64_t value, const uint64_t step) __const
{
    return ((value + step - 1) / step) * step;
}
__forceinline uint32_t RoundUp32(const uint32_t value, const uint32_t step) __const
{
    return ((value + step - 1) / step) * step;
}
#else
__forceinline uint64_t RoundUp64(const uint64_t value, const uint64_t step) __const
{
    return value + ((step - (value % step)) % step);
}
__forceinline uint32_t RoundUp32(const uint32_t value, const uint32_t step) __const
{
    return value + ((step - (value % step)) % step);
}
#endif

__forceinline uint64_t RoundDown64(const uint64_t value, const uint64_t step) __const
{
    return value - (value & step);
}
__forceinline uint32_t RoundDown32(const uint32_t value, const uint32_t step) __const
{
    return value - (value & step);
}

__forceinline uint64_t RoundUpDiff64(const uint64_t value, const uint64_t step) __const
{
    return (step - (value % step)) % step;
}
__forceinline uint32_t RoundUpDiff32(const uint32_t value, const uint32_t step) __const
{
    return (step - (value % step)) % step;
}

 #define MIN(aP, bP)             \
   ({  __typeof__ (a) _a = (aP); \
       __typeof__ (b) _b = (bP); \
       _a < _b ? _a : _b;        })
 #define MAX(aP, bP)             \
   ({  __typeof__ (a) _a = (aP); \
       __typeof__ (b) _b = (bP); \
       _a > _b ? _a : _b;        })
//  Courtesy of http://stackoverflow.com/a/3437484/485098

__forceinline uint64_t GreatestCommonDivisor64(uint64_t a, uint64_t b) __const
{
    uint64_t t;

    while (b != 0)
    {
        t = b;
        b = a % b;
        a = t;
    }

    return a;
}

__forceinline uint64_t GreatestCommonDivisor32(uint32_t a, uint32_t b) __const
{
    uint32_t t;

    while (b != 0)
    {
        t = b;
        b = a % b;
        a = t;
    }

    return a;
}

#endif

__shared __const uint8_t Log2_32(uint32_t val);
__shared __const uint8_t Log2_64(uint64_t val);

__shared __const uint32_t AddWithCarry32(uint32_t * dst, uint32_t src, uint32_t cin);
__shared __const uint32_t AddWithCarry32_3(uint32_t * dst, uint32_t src1, uint32_t src2, uint32_t cin);

__shared __const uint32_t AddWithCarry64(uint64_t * dst, uint64_t src, uint32_t cin);

struct PointerAndSize
{
    uintptr_t Start;
    size_t Size;
};

__shared __const struct PointerAndSize IntersectMemoryRanges(struct PointerAndSize a, struct PointerAndSize b);
__shared __const bool DoRangesIntersect(struct PointerAndSize a, struct PointerAndSize b);

#ifdef __cplusplus
}
#endif
