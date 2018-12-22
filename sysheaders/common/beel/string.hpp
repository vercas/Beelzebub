/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

#include <beel/metaprogramming.h>

namespace Beelzebub
{
    typedef unsigned long long strhash_t;

    class CStringUtils
    {
    public:
        /*  Constants  */

        static constexpr strhash_t const Prime = 0x100000001B3ull;
        static constexpr strhash_t const Basis = 0xCBF29CE484222325ull;

    protected:
        /*  Constructor(s)  */

        CStringUtils() = default;

    public:
        CStringUtils(CStringUtils const &) = delete;
        CStringUtils & operator =(CStringUtils const &) = delete;

        static inline strhash_t Hash(char const * str)
        {
            if unlikely(str == nullptr)
                return 0;

            strhash_t ret = Basis;
         
            while (*(str++))
            {
                ret ^= *str;
                ret *= Prime;
            }
         
            return ret;
        }

        static inline constexpr strhash_t HashConstexprInner(char const * str, strhash_t last_value)
        {
            return *str ? HashConstexprInner(str + 1, (*str ^ last_value) * Prime) : last_value;
        }

        static inline constexpr strhash_t HashConstexpr(char const * str)
        {
            return str == nullptr ? 0 : HashConstexprInner(str, Basis);
        }
    };
}

static inline constexpr unsigned long long operator""BeH(char const * str, size_t)
{
    return Beelzebub::CStringUtils::HashConstexpr(str);
}
