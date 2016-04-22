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

#include <utils/comparables.hpp>
#include <string.h>

namespace Beelzebub { namespace Utils
{
    #define COMP_INT_1(TThis) \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TThis>(TThis const & other) const \
    { \
        return static_cast<comp_t>(this->Object) - static_cast<comp_t>(other); \
    } \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TThis>(TThis const && other) const \
    { \
        return static_cast<comp_t>(this->Object) - static_cast<comp_t>(other); \
    }

    #define COMP_INT_2(TThis, TOther) \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TOther>(TOther const & other) const \
    { \
        return static_cast<comp_t>(this->Object) - static_cast<comp_t>(other); \
    } \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TOther>(TOther const && other) const \
    { \
        return static_cast<comp_t>(this->Object) - static_cast<comp_t>(other); \
    } \
    template<> template<> \
    comp_t Comparable<TOther>::Compare<TThis>(TThis const & other) const \
    { \
        return static_cast<comp_t>(this->Object) - static_cast<comp_t>(other); \
    } \
    template<> template<> \
    comp_t Comparable<TOther>::Compare<TThis>(TThis const && other) const \
    { \
        return static_cast<comp_t>(this->Object) - static_cast<comp_t>(other); \
    }

#define COMP_INT(...) GET_MACRO2(__VA_ARGS__, COMP_INT_2, COMP_INT_1)(__VA_ARGS__)

    #define COMP_COVER_1(TThis) \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TThis>(TThis const & other) const \
    { \
        return (this->Object == other) ? 0 : ((this->Object < other) ? -1 : 1); \
    } \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TThis>(TThis const && other) const \
    { \
        return (this->Object == other) ? 0 : ((this->Object < other) ? -1 : 1); \
    }

    #define COMP_COVER_2(TThis, TOther) \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TOther>(TOther const & other) const \
    { \
        return (this->Object == other) ? 0 : ((this->Object < other) ? -1 : 1); \
    } \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TOther>(TOther const && other) const \
    { \
        return (this->Object == other) ? 0 : ((this->Object < other) ? -1 : 1); \
    } \
    template<> template<> \
    comp_t Comparable<TOther>::Compare<TThis>(TThis const & other) const \
    { \
        return (this->Object == other) ? 0 : ((this->Object < other) ? -1 : 1); \
    } \
    template<> template<> \
    comp_t Comparable<TOther>::Compare<TThis>(TThis const && other) const \
    { \
        return (this->Object == other) ? 0 : ((this->Object < other) ? -1 : 1); \
    }

#define COMP_COVER(...) GET_MACRO2(__VA_ARGS__, COMP_COVER_2, COMP_COVER_1)(__VA_ARGS__)

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

    COMP_COVER(float)
    COMP_COVER(double)
    COMP_COVER(long double)

    COMP_COVER(float, double)
    COMP_COVER(float, long double)

    COMP_COVER(double, long double)

#pragma GCC diagnostic pop

    #define COMP_STR_1(TThis) \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TThis>(TThis const & other) const \
    { \
        return strcmp(const_cast<char const *>(this->Object) \
                    , const_cast<char const *>(other       )); \
    } \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TThis>(TThis const && other) const \
    { \
        return strcmp(const_cast<char const *>(this->Object) \
                    , const_cast<char const *>(other       )); \
    }

    #define COMP_STR_2(TThis, TOther) \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TOther>(TOther const & other) const \
    { \
        return strcmp(const_cast<char const *>(this->Object) \
                    , const_cast<char const *>(other       )); \
    } \
    template<> template<> \
    comp_t Comparable<TThis>::Compare<TOther>(TOther const && other) const \
    { \
        return strcmp(const_cast<char const *>(this->Object) \
                    , const_cast<char const *>(other       )); \
    } \
    template<> template<> \
    comp_t Comparable<TOther>::Compare<TThis>(TThis const & other) const \
    { \
        return strcmp(const_cast<char const *>(this->Object) \
                    , const_cast<char const *>(other       )); \
    } \
    template<> template<> \
    comp_t Comparable<TOther>::Compare<TThis>(TThis const && other) const \
    { \
        return strcmp(const_cast<char const *>(this->Object) \
                    , const_cast<char const *>(other       )); \
    }

    #define COMP_STR(...) GET_MACRO2(__VA_ARGS__, COMP_STR_2, COMP_STR_1)(__VA_ARGS__)

    COMP_STR(char *)
    COMP_STR(char const *)
    COMP_STR(char *, char const *)
}}
