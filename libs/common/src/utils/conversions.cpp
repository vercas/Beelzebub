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

#include <beel/utils/conversions.hpp>
#include <string.h>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

/*  Workers  */

template<typename TInt, size_t limit>
static Handle StrToUIntBase16(char const * str, TInt & val)
{
    TInt res = 0;
    size_t i = 0;

    for (char c; (c = *str) != '\0' && i < limit; ++i, ++str)
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
                        res |= (TInt)(c - 0x60);

                        continue;
                    }
                }
                else if (c <= 0x46)
                {
                    res |= (TInt)(c - 0x40);

                    continue;
                }
            }
            else if (c < 0x3A)
            {
                res |= (TInt)(c - 0x30);

                continue;
            }
        }

        //  Reaching this point means invalid digit.

        return HandleResult::ArgumentOutOfRange;
    }

    if (i >= limit || i == 0)
        return HandleResult::ArgumentOutOfRange;

    val = res;

    return HandleResult::Okay;
}

template<typename TInt>
static Handle StrToUIntBase10(char const * str, TInt & val)
{
    TInt res = val = 0;
    char c;

    for (size_t i = 0; (c = *str) != '\0'; ++i, ++str)
    {
        res *= 10;
        //  Shift by one decimal digit. I hope the compiler optimises this into
        //  two shifts and addition.

        if (res < val)
            return HandleResult::ArgumentOutOfRange;
        //  This indicates overflow. It may also kinda allow some overflow.

        val = res;

        if (c < 0x30 || c >= 0x3A)
            return HandleResult::ArgumentOutOfRange;
        //  So not a decimal digit.

        res += (TInt)(c - 0x30);
    }

    val = res;

    return HandleResult::Okay;
}

template<typename TInt, size_t limit>
static Handle StrToUIntBase8(char const * str, TInt & val)
{
    TInt res = 0;
    size_t i = 0;

    for (char c; (c = *str) != '\0' && i < limit; ++i, ++str)
    {
        res <<= 3;
        //  Shift by one octal digit.

        if (c < 0x30 || c >= 0x38)
            return HandleResult::ArgumentOutOfRange;
        //  So not an octal digit.

        res |= (TInt)(c - 0x30);
    }

    if (i >= limit || i == 0)
        return HandleResult::ArgumentOutOfRange;

    val = res;

    return HandleResult::Okay;
}

template<typename TInt>
static Handle StrToUIntBase2(char const * str, TInt & val)
{
    TInt res = 0;
    size_t i = 0;

    for (char c; (c = *str) != '\0' && i < (sizeof(TInt) * 8); ++i, ++str)
        if (c == '1')
            res = (res << 1) | 1;                       //  Shift by one binary digit and set the low one.
        else if (c == '0')
            res <<= 1;                                  //  Shift one bit; low one's zero.
        else
            return HandleResult::ArgumentOutOfRange;    //  So not a binary digit.

    if (i >= (sizeof(TInt) * 8) || i == 0)
        return HandleResult::ArgumentOutOfRange;

    val = res;

    return HandleResult::Okay;
}

/*  The juice!  */

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
                //  Hexadecimal (base 16).
                res = StrToUIntBase16<uint64_t, 16>(str + 2, temp);
            else if (str[1] == 'b' || str[1] == 'B')
                //  Binary (base 2).
                res = StrToUIntBase2<uint64_t>(str + 2, temp);
            else
                //  Octal (base 8).
                res = StrToUIntBase8<uint64_t, 21>(str + 1, temp);
        }
        else
            //  Decimal (base 10).
            res = StrToUIntBase10<uint64_t>(str, temp);

        if (!res.IsOkayResult())
            return res;

        if (temp >> 63 != 0)
            return HandleResult::ArgumentOutOfRange;

        if (negative)
            val = -static_cast<int64_t>(temp);
        else
            val = static_cast<int64_t>(temp);

        return res;
    }

    template<>
    Handle FromString<uint64_t>(char const * str, uint64_t & val)
    {
        if (*str == '0')
        {
            //  So... The base ain't 10.

            if (str[1] == 'x' || str[1] == 'X')
            {
                //  Hexadecimal (base 16).
                
                return StrToUIntBase16<uint64_t, 16>(str + 2, val);
            }
            else if (str[1] == 'b' || str[1] == 'B')
            {
                //  Binary (base 2).
                
                return StrToUIntBase2<uint64_t>(str + 2, val);
            }
            else
            {
                //  Octal (base 8).
                
                return StrToUIntBase8<uint64_t, 21>(str + 1, val);
            }
        }
        else
        {
            //  Decimal (base 10).
            
            return StrToUIntBase10<uint64_t>(str, val);
        }
    }

    template<>
    Handle FromString<int32_t>(char const * str, int32_t & val)
    {
        Handle res;
        uint32_t temp;
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
                
                res = StrToUIntBase16<uint32_t, 16>(str + 2, temp);
            }
            else if (str[1] == 'b' || str[1] == 'B')
            {
                //  Binary (base 2).
                
                res = StrToUIntBase2<uint32_t>(str + 2, temp);
            }
            else
            {
                //  Octal (base 8).

                uint64_t temp2;
                
                res = StrToUIntBase8<uint64_t, 11>(str + 1, temp2);

                if ((temp2 & 0xFFFFFFFFULL) != 0)
                    return HandleResult::ArgumentOutOfRange;

                temp = static_cast<uint32_t>(temp2);
            }
        }
        else
        {
            //  Decimal (base 10).
            
            res = StrToUIntBase10<uint32_t>(str, temp);
        }

        if (!res.IsOkayResult())
            return res;

        if (temp >> 31 != 0)
            return HandleResult::ArgumentOutOfRange;

        if (negative)
            val = -static_cast<int32_t>(temp);
        else
            val = static_cast<int32_t>(temp);

        return res;
    }

    template<>
    Handle FromString<uint32_t>(char const * str, uint32_t & val)
    {
        if (*str == '0')
        {
            //  So... The base ain't 10.

            if (str[1] == 'x' || str[1] == 'X')
            {
                //  Hexadecimal (base 16).
                
                return StrToUIntBase16<uint32_t, 16>(str + 2, val);
            }
            else if (str[1] == 'b' || str[1] == 'B')
            {
                //  Binary (base 2).
                
                return StrToUIntBase2<uint32_t>(str + 2, val);
            }
            else
            {
                //  Octal (base 8).

                uint64_t temp;
                
                Handle res = StrToUIntBase8<uint64_t, 11>(str + 1, temp);

                if (res.IsOkayResult() && (temp & 0xFFFFFFFFULL) != 0)
                    return HandleResult::ArgumentOutOfRange;

                val = temp;

                return res;
            }
        }
        else
        {
            //  Decimal (base 10).

            return StrToUIntBase10<uint32_t>(str, val);
        }
    }

    template<>
    Handle FromString<bool>(char const * str, bool & val)
    {
        //  6 chars ought to be enough.
        char buf[6] = "\0\0\0\0\0";
        size_t i = 0;

        for (char c; i < 6 && (c = str[i]) != '\0'; ++i)
            if (c >= 'A' && c <= 'Z')
                buf[i] = c + 32;
            else
                buf[i] = c;

        //  Yes, it converts it all to lowercase.
        //  Also, an on-stack buffer is used here so the source string isn't
        //  modified.

        if unlikely(i == 6)
            return HandleResult::ArgumentOutOfRange;
        //  Too many characters.

        //  Also, case-insensitive comparisons could've been used, but many
        //  may be needed, which are much faster when using same-case comparisons.

        #define CHECKSTR(strval, boolval)                           \
            if (memeq(buf, strval, i) && i == sizeof(strval) - 1)   \
                val = boolval;                                      \
            else
        //  Yes, this is highly unorthodox. Blimey.

        CHECKSTR("true", true)
        CHECKSTR("false", false)

        CHECKSTR("yes", true)
        CHECKSTR("no", false)

        CHECKSTR("on", true)
        CHECKSTR("off", false)

        /* else */ 
            return HandleResult::ArgumentOutOfRange;

        //  Please pay attention to that awful macro to understand this code.

        return HandleResult::Okay;
    }
}}
