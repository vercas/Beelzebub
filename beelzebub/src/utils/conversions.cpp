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

#include <utils/conversions.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

Handle StrToUInt64Base16(char const * str, uint64_t & val)
{
    uint64_t res = 0;
    size_t i = 0;

    for (char c; (c = *str) != '\0' && i < 16; ++i, ++str)
    {
        res <<= 4;
        //  Shift by one hexadecimal digit.

        if (c >= 0x30)
        {
            if (c > 0x40)
            {
                if (c > 0x60)
                {
                    if (c <= 0x66)
                    {
                        res |= (uint64_t)(c - 0x60);

                        continue;
                    }
                }
                else if (c <= 0x46)
                {
                    res |= (uint64_t)(c - 0x40);

                    continue;
                }
            }
            else if (c < 0x3A)
            {
                res |= (uint64_t)(c - 0x30);

                continue;
            }
        }

        //  Reaching this point means invalid digit.

        return HandleResult::ArgumentOutOfRange;
    }

    if (i >= 16 || i == 0)
        return HandleResult::ArgumentOutOfRange;

    val = res;

    return HandleResult::Okay;
}

Handle StrToUInt64Base10(char const * str, uint64_t & val)
{
    uint64_t res = val = 0;

    for (size_t i = 0; *str != '\0'; ++i, ++str)
    {
        res *= 10;
        //  Shift by one decimal digit. I hope the compiler optimises this into
        //  two shifts and addition.

        if (res < val)
            return HandleResult::ArgumentOutOfRange;
        //  This indicates overflow. It may also kinda allow some overflow.

        val = res;

        if (c < 0x30 || c >= 0x3A)
        {
            //  So not a decimal digit.

            return HandleResult::ArgumentOutOfRange;
        }

        res += (uint64_t)(c - 0x30);
    }

    val = res;

    return HandleResult::Okay;
}

Handle StrToUInt64Base8(char const * str, uint64_t & val)
{
    uint64_t res = 0;
    size_t i = 0;

    for (char c; (c = *str) != '\0' && i < 21; ++i, ++str)
    {
        res <<= 3;
        //  Shift by one octal digit.

        if (c < 0x30 || c >= 0x38)
        {
            //  So not an octal digit.

            return HandleResult::ArgumentOutOfRange;
        }

        res |= (uint64_t)(c - 0x30);
    }

    if (i >= 21 || i == 0)
        return HandleResult::ArgumentOutOfRange;

    val = res;

    return HandleResult::Okay;
}

Handle StrToUInt64Base2(char const * str, uint64_t & val)
{
    uint64_t res = 0;
    size_t i = 0;

    for (char c; (c = *str) != '\0' && i < 64; ++i, ++str)
    {
        res <<= 1;
        //  Shift by one octal digit.

        if (c == 0x31)
            res |= 1;
        else if (c != 0x30)
        {
            //  So not a binary digit.

            return HandleResult::ArgumentOutOfRange;
        }
    }

    if (i >= 64 || i == 0)
        return HandleResult::ArgumentOutOfRange;

    val = res;

    return HandleResult::Okay;
}

namespace Beelzebub { namespace Utils
{
    template<>
    Handle FromString<int64_t>(char const * str, int64_t & val)
    {
        Handle res;
        uint64_t temp;
        bool negative = false;

        if (*str == '-')
        {
            ++str;
            negative = true;
        }

        if (*str == '0')
        {
            //  So... The base ain't 10.

            if (str[1] == 'x' || str[1] == 'X')
            {
                //  Hexadecimal (base 16).
                
                res = StrToUInt64Base16(str + 2, temp);
            }
            else if (str[1] == 'b' || str[1] == 'B')
            {
                //  Binary (base 2).
                
                res = StrToUInt64Base2(str + 2, temp);
            }
            else
            {
                //  Octal (base 8).
                
                res = StrToUInt64Base8(str + 1, temp);
            }
        }
        else
        {
            //  Decimal (base 10).
            
            res = StrToUInt64Base10(str, temp);
        }

        if (!res.IsOkayResult())
            return res;

        if (temp >> 63 != 0)
        {
            msg("!! top bit set !!%n");

            return HandleResult::ArgumentOutOfRange;
        }

        if (negative)
            val = -static_cast<int64_t>(temp);
        else
            val = static_cast<int64_t>(temp);

        return res;
    }
}}
