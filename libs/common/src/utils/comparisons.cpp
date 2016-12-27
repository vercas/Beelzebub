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

#include <beel/utils/comparisons.hpp>
#include <string.h>

namespace Beelzebub { namespace Utils
{
    //  Comparison by integral subtraction

    #define COMP_INT_IMPL(a, b) return static_cast<comp_t>(a) - static_cast<comp_t>(b);

    #define COMP_INT_1(TThis) \
        COMP_IMPL(TThis, TThis, COMP_INT_IMPL)

    #define COMP_INT_2(TThis, TOther) \
        COMP_IMPL(TThis, TOther, COMP_INT_IMPL) \
        COMP_IMPL(TOther, TThis, COMP_INT_IMPL)

    #define COMP_INT(...) GET_MACRO2(__VA_ARGS__, COMP_INT_2, COMP_INT_1)(__VA_ARGS__)

    //  Comparison covered by operator implementations

    #define COMP_COVER_IMPL(a, b) return (a == b) ? 0 : ((a < b) ? -1 : 1);

    #define COMP_COVER_1(TThis) \
        COMP_IMPL(TThis, TThis, COMP_COVER_IMPL)

    #define COMP_COVER_2(TThis, TOther) \
        COMP_IMPL(TThis, TOther, COMP_COVER_IMPL) \
        COMP_IMPL(TOther, TThis, COMP_COVER_IMPL)

    #define COMP_COVER(...) GET_MACRO2(__VA_ARGS__, COMP_COVER_2, COMP_COVER_1)(__VA_ARGS__)

    //  Comparison of integral types with themselves.

    COMP_INT(char)
    COMP_INT(signed char)
    COMP_INT(unsigned char)

    COMP_INT(short)
    COMP_INT(unsigned short)

    COMP_INT(int)
    COMP_COVER(unsigned int)

    COMP_COVER(long)
    COMP_COVER(unsigned long)

    COMP_COVER(long long)
    COMP_COVER(unsigned long long)

    //  Comparison of integral types with other types.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"

    COMP_INT(char, signed char)
    COMP_INT(char, unsigned char)

    COMP_INT(char, short)
    COMP_INT(char, unsigned short)

    COMP_INT(char, int)
    COMP_COVER(char, unsigned int)

    COMP_COVER(char, long)
    COMP_COVER(char, unsigned long)

    COMP_COVER(char, long long)
    COMP_COVER(char, unsigned long long)

    COMP_INT(signed char, unsigned char)

    COMP_INT(signed char, short)
    COMP_INT(signed char, unsigned short)

    COMP_INT(signed char, int)
    COMP_COVER(signed char, unsigned int)

    COMP_COVER(signed char, long)
    COMP_COVER(signed char, unsigned long)

    COMP_COVER(signed char, long long)
    COMP_COVER(signed char, unsigned long long)

    COMP_INT(unsigned char, short)
    COMP_INT(unsigned char, unsigned short)

    COMP_INT(unsigned char, int)
    COMP_COVER(unsigned char, unsigned int)

    COMP_COVER(unsigned char, long)
    COMP_COVER(unsigned char, unsigned long)

    COMP_COVER(unsigned char, long long)
    COMP_COVER(unsigned char, unsigned long long)

    COMP_INT(short, unsigned short)

    COMP_INT(short, int)
    COMP_COVER(short, unsigned int)

    COMP_COVER(short, long)
    COMP_COVER(short, unsigned long)

    COMP_COVER(short, long long)
    COMP_COVER(short, unsigned long long)

    COMP_INT(unsigned short, int)
    COMP_COVER(unsigned short, unsigned int)

    COMP_COVER(unsigned short, long)
    COMP_COVER(unsigned short, unsigned long)

    COMP_COVER(unsigned short, long long)
    COMP_COVER(unsigned short, unsigned long long)

    COMP_COVER(int, unsigned int)

    COMP_COVER(int, long)
    COMP_COVER(int, unsigned long)

    COMP_COVER(int, long long)
    COMP_COVER(int, unsigned long long)

    COMP_COVER(unsigned int, long)
    COMP_COVER(unsigned int, unsigned long)

    COMP_COVER(unsigned int, long long)
    COMP_COVER(unsigned int, unsigned long long)

    COMP_COVER(long, unsigned long)

    COMP_COVER(long, long long)
    COMP_COVER(long, unsigned long long)

    COMP_COVER(unsigned long, long long)
    COMP_COVER(unsigned long, unsigned long long)

    COMP_COVER(long long, unsigned long long)

    //  Comparison of floating point types with themselves.

    COMP_COVER(float)
    COMP_COVER(double)
    COMP_COVER(long double)

    //  Comparison of floating point types with other types.

    COMP_COVER(float, double)
    COMP_COVER(float, long double)

    COMP_COVER(double, long double)

    //  Comparisons of floating point types with integral types.

    COMP_COVER(float, char)
    COMP_COVER(float, signed char)
    COMP_COVER(float, unsigned char)

    COMP_COVER(float, short)
    COMP_COVER(float, unsigned short)

    COMP_COVER(float, int)
    COMP_COVER(float, unsigned int)

    COMP_COVER(float, long)
    COMP_COVER(float, unsigned long)

    COMP_COVER(float, long long)
    COMP_COVER(float, unsigned long long)

    COMP_COVER(double, char)
    COMP_COVER(double, signed char)
    COMP_COVER(double, unsigned char)

    COMP_COVER(double, short)
    COMP_COVER(double, unsigned short)

    COMP_COVER(double, int)
    COMP_COVER(double, unsigned int)

    COMP_COVER(double, long)
    COMP_COVER(double, unsigned long)

    COMP_COVER(double, long long)
    COMP_COVER(double, unsigned long long)

    COMP_COVER(long double, char)
    COMP_COVER(long double, signed char)
    COMP_COVER(long double, unsigned char)

    COMP_COVER(long double, short)
    COMP_COVER(long double, unsigned short)

    COMP_COVER(long double, int)
    COMP_COVER(long double, unsigned int)

    COMP_COVER(long double, long)
    COMP_COVER(long double, unsigned long)

    COMP_COVER(long double, long long)
    COMP_COVER(long double, unsigned long long)

#pragma GCC diagnostic pop

    //  Comparison of strings.

    #define COMP_STR_IMPL(a, b) return strcmp(const_cast<char const *>(a) \
                                            , const_cast<char const *>(b));

    #define COMP_STR_1(TThis) \
        COMP_IMPL(TThis, TThis, COMP_STR_IMPL)

    #define COMP_STR_2(TThis, TOther) \
        COMP_IMPL(TThis, TOther, COMP_STR_IMPL) \
        COMP_IMPL(TOther, TThis, COMP_STR_IMPL)

    #define COMP_STR(...) GET_MACRO2(__VA_ARGS__, COMP_STR_2, COMP_STR_1)(__VA_ARGS__)

    COMP_STR(char *)
    COMP_STR(char const *)
    COMP_STR(char *, char const *)
}}
