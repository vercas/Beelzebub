/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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

namespace Beelzebub
{
    struct TimeSpanLite
    {
        inline constexpr TimeSpanLite() : Value( 0) { }
        inline constexpr explicit TimeSpanLite(uint64_t const val) : Value( val) { }

        inline TimeSpanLite operator +(TimeSpanLite const other)
        { return TimeSpanLite(this->Value + other.Value); }

        inline TimeSpanLite operator -(TimeSpanLite const other)
        { return TimeSpanLite(this->Value + other.Value); }

        #define CMPOP(OP) \
        inline bool operator OP(TimeSpanLite const other) \
        { return this->Value OP other.Value; }

        CMPOP(<) CMPOP(<=) CMPOP(>) CMPOP(>=) CMPOP(==) CMPOP(!=)
        #undef CMPOP

        uint64_t Value;
    };

    struct TimeInstantLite
    {
        inline constexpr TimeInstantLite() : Value( 0) { }
        inline constexpr explicit TimeInstantLite(uint64_t const val) : Value( val) { }

        inline TimeInstantLite operator +(TimeSpanLite const other)
        { return TimeInstantLite(this->Value + other.Value); }

        inline TimeInstantLite operator -(TimeSpanLite const other)
        { return TimeInstantLite(this->Value + other.Value); }

        #define CMPOP(OP) \
        inline bool operator OP(TimeInstantLite const other) \
        { return this->Value OP other.Value; }

        CMPOP(<) CMPOP(<=) CMPOP(>) CMPOP(>=) CMPOP(==) CMPOP(!=)
        #undef CMPOP

        uint64_t Value;
    };

    struct TimeSpan
    {
        inline constexpr TimeSpan() : Value( 0) { }
        inline constexpr explicit TimeSpan(int128_t const val) : Value( val) { }
        inline constexpr TimeSpan(TimeSpanLite const val) : Value((uint128_t)val.Value * 1'000'000'000'000ULL) { }

        inline TimeSpan operator +(TimeSpan const other)
        { return TimeSpan(this->Value + other.Value); }

        inline TimeSpan operator +(TimeSpanLite const other)
        { return TimeSpan(this->Value + (uint128_t)other.Value * 1'000'000'000'000ULL); }

        inline TimeSpan operator -(TimeSpan const other)
        { return TimeSpan(this->Value + other.Value); }

        inline TimeSpan operator -(TimeSpanLite const other)
        { return TimeSpan(this->Value + (uint128_t)other.Value * 1'000'000'000'000ULL); }

        uint128_t Value;
    };

    struct TimeInstant
    {
        inline constexpr TimeInstant() : Value( 0) { }
        inline constexpr explicit TimeInstant(int128_t const val) : Value( val) { }

        inline TimeInstant operator +(TimeSpan const other)
        { return TimeInstant(this->Value + other.Value); }

        inline TimeInstant operator +(TimeSpanLite const other)
        { return TimeInstant(this->Value + (uint128_t)other.Value * 1'000'000'000'000ULL); }

        inline TimeInstant operator -(TimeSpan const other)
        { return TimeInstant(this->Value + other.Value); }

        inline TimeInstant operator -(TimeSpanLite const other)
        { return TimeInstant(this->Value + (uint128_t)other.Value * 1'000'000'000'000ULL); }

        int128_t Value;
    };

    inline constexpr TimeSpan operator "" secs(unsigned long long val)
    { return TimeSpan(val * 1'000'000'000'000'000'000ULL); }

    inline constexpr TimeSpanLite operator "" secs_l(unsigned long long val)
    { return TimeSpanLite(val * 1'000'000UL); }
}
