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

#include <utils/utf8.h>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

decoded_codepoint_t Utils::GetUtf8Codepoint(char const * chr)
{
    decoded_codepoint_t res;
    char c1 = *chr;

    if (c1 >= 0xE0 /* 0b1110_xxxx */)
    {
        if likely(c1 < 0xF8 /* 0b1111_10xx */)
        {
            if likely(c1 < 0xF0 /* 0b1110_xxxx */)
            {
                res.Next = const_cast<char *>(chr) + 3;
                res.Char = c1 & 0x0F;
            }
            else /* 0b1111_0xxx */
            {
                res.Next = const_cast<char *>(chr) + 4;
                res.Char = c1 & 0x07;
            }
        }
        else
        {
            if (c1 >= 0xFC /* 0b1111_110x */)
            {
                res.Next = const_cast<char *>(chr) + 6;
                res.Char = c1 & 0x01;
            }
            else /* 0b1111_10xx */
            {
                res.Next = const_cast<char *>(chr) + 5;
                res.Char = c1 & 0x03;
            }
        }
    }
    else
    {
        if (c1 >= 0xC0 /* 0b110x_xxxx */)
        {
            res.Next = const_cast<char *>(chr) + 2;
            res.Char = c1 & 0x1F;
        }
        else if likely(c1 < 0x80 /* 0b10xx_xxxx */)
        {
            res.Next = const_cast<char *>(chr) + 1;
            res.Char = c1 & 0x7F;

            return res;
        }
        else /* 0b0xxx_xxxx */
            goto fail;
    }

    do
    {
        char c = *(++chr);

        if unlikely((c & 0xC0) != 0x80)
            goto fail;

        res.Char = (res.Char << 6) | (c & 0x3F);
    }
    while (chr < res.Next);

    return res;

fail:
    res.Next = nullptr;
    res.Char = __WCHAR_MAX__;

    return res;
}
