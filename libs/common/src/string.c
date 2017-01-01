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

/**
 *  The loops here should be perfectly compatible with single machine code
 *  instructions. Maybe with a branch or two mixed in.
 */

#include <string.h>

void * memchr(void const * src, int val, size_t len)
{
//     asm volatile ( "repne scasb   \n\t"
//                    "cmovne %0, %1 \n\t"
//                  : "+D"(src), "+c"(len)
//                  : "a"(val)
//                  : "memory" );
//     //  `ret` started at `len`, and is decremented once for every character
//     //  ecountered up to 0 inclusively. Therefore, `ret` will contain
//     //  `len - actual length` after the assembly block. `~ret` flips all the
//     //  bits, so I return `len - ret`.

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
//     return src == nullptr ? nullptr : ((uint8_t *)src) - 1;
// #pragma GCC diagnostic pop

    uint8_t const * s = (uint8_t const *)src;
    uint8_t const bVal = val & 0xFF;

    for (; len > 0; ++s, --len)
        if (*s == bVal)
            return (void *)s;   //  Need to discard the `const` qualifier.

    return nullptr; //*/
}

char * strcat(char * dst, char const * src)
{
    char * tmp = dst + strlen(dst), c;
    //  Now *tmp is the null terminator.

    do
    {
        c = *(src++);
        *(tmp++) = c;
    } while (c != 0);

    return dst;
}

char * strncat(char * dst, char const * src, size_t len)
{
    char * tmp = dst + strlen(dst), c;
    //  Now *tmp is the null terminator.

    do
    {
        c = *(src++);
        *(tmp++) = c;
    } while (c != 0 && --len > 0);

    if (c != 0)
        *tmp = (char)0;
    //  Gotta have a terminating null character.

    return dst;
}

char * strcpy(char * dst, char const * src)
{
    char * tmp = dst, c;
    //  Now *tmp is the null terminator.

    do
    {
        c = *(src++);
        *(tmp++) = c;
    } while (c != 0);

    return dst;
}

char * strncpy(char * dst, char const * src, size_t len)
{
    char * tmp = dst, c;
    //  Now *tmp is the null terminator.

    do
    {
        c = *(src++);
        *(tmp++) = c;
    } while (c != 0 && --len > 0);

    return dst;
}

char * strpbrk(char const * haystack, char const * needle)
{
    size_t nLen = strlen(needle);
    char c;

    do
    {
        c = *(haystack++);

        if (memchr(needle, c, nLen) != nullptr)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
            return haystack - 1;
#pragma GCC diagnostic pop
    } while (c != 0);

    return nullptr;
}

char * strnpbrk(char const * haystack, char const * needle, size_t len)
{
    size_t nLen = strlen(needle);
    char c;

    do
    {
        c = *(haystack++);

        if (memchr(needle, c, nLen) != nullptr)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
            return haystack - 1;
#pragma GCC diagnostic pop
    } while (c != 0 && --len > 0);

    return nullptr;
}

comp_t strcmp(char const * src1, char const * src2)
{
    comp_t res;
    char c1, c2;

    if (src1 == src2)
        return 0;

    do
    {
        c1 = *(src1++);
        c2 = *(src2++);

        res = (comp_t)((int8_t)c1 - (int8_t)c2);
    } while (res == 0 && ((c1 != 0) || (c2 != 0)));

    return res;
}

comp_t strncmp(char const * src1, char const * src2, size_t len)
{
    comp_t res = 0;    //  Used to store subtraction/comparison results.
    char c1, c2;

    if (src1 == src2)
        return 0;

    do
    {
        c1 = *(src1++);
        c2 = *(src2++);

        res = (comp_t)((int8_t)c1 - (int8_t)c2);
    } while (res == 0 && --len > 0 && ((c1 != 0) || (c2 != 0)));

    return res;
}

comp_t strcasecmp(char const * src1, char const * src2)
{
    comp_t res;
    char c1, c2;

    if (src1 == src2)
        return 0;

    do
    {
        c1 = *(src1++);
        c2 = *(src2++);

        res = (comp_t)((int8_t)c1 - (int8_t)c2);

        if unlikely(res != 0)
        {
            //  Considering this is ASCII, the only case where we need to continue
            //  is when the characters are identical letters with opposite casing.
            //  In other words, their difference is +/- 32 and either one of them
            //  is in the right letter range.

            if (res == 32 && c1 >= 'a' && c1 <= 'z')
            {
                res = 0;    //  In case it breaks.

                continue;
            }
            //  c1 - c2 = 32 means c1 must be lowercase.

            if (res == -32 && c1 >= 'A' && c1 <= 'A')
            {
                res = 0;

                continue;
            }
            //  -32 means it has to be uppercase.

            return res;
        }
    } while ((c1 != 0) || (c2 != 0));

    //  This is just odd...

    return res;
}

comp_t strcasencmp(char const * src1, char const * src2, size_t len)
{
    comp_t res;
    char c1, c2;

    if (src1 == src2)
        return 0;

    do
    {
        c1 = *(src1++);
        c2 = *(src2++);

        res = (comp_t)((int8_t)c1 - (int8_t)c2);

        if unlikely(res != 0)
        {
            if (res == 32 && c1 >= 'a' && c1 <= 'z')
            {
                res = 0;

                continue;
            }

            if (res == -32 && c1 >= 'A' && c1 <= 'A')
            {
                res = 0;

                continue;
            }

            return res;
        }
    } while (--len > 0 && ((c1 != 0) || (c2 != 0)));

    return res;
}

char * strchr(char const * haystack, int needle)
{
    char c;

    while ((c = *(haystack++)) != '\0')
        if (c == needle)
            return (char *)haystack - 1;

    return nullptr;
}

char * strstr(char const * haystack, char const * needle)
{
    size_t hLen = strlen(haystack), nLen = strlen(needle);

    if (hLen < nLen)
        return nullptr;
    //  No way the needle can be larger than the haystack.

    size_t lenDiff = hLen - nLen;

    for (size_t i = 0; i <= lenDiff; ++i)
        if (memeq(haystack + i, needle, nLen))
            return (char *)(haystack + i);

    return nullptr;
}

char const * strstrex(char const * haystack, char const * needle, char const * seps)
{
    size_t hLen = strlen(haystack), nLen = strlen(needle);

    if (hLen < nLen)
        return nullptr;
    //  No way the needle can be larger than the haystack.

    size_t lenDiff = hLen - nLen, sLen = strlen(seps) + 1;
    //  sLen will include the null terminator.

    for (size_t i = 0; i <= lenDiff; /* nothing */)
    {
        size_t j = i - 1;

        while (memchr(seps, haystack[++j], sLen) == nullptr) ;
        //  Basically finds the separator.

        if (j - i == nLen && memeq(haystack + i, needle, nLen))
            return haystack + i;

        i = j + 1;
    }

    return nullptr;
}

char const * strcasestrex(char const * haystack, char const * needle, char const * seps)
{
    size_t hLen = strlen(haystack), nLen = strlen(needle);

    if (hLen < nLen)
        return nullptr;
    //  No way the needle can be larger than the haystack.

    size_t lenDiff = hLen - nLen, sLen = strlen(seps) + 1;
    //  sLen will include the null terminator.

    for (size_t i = 0; i <= lenDiff; /* nothing */)
    {
        size_t j = i - 1;

        while (memchr(seps, haystack[++j], sLen) == nullptr) ;
        //  Basically finds the separator.

        if (j - i == nLen && strcasencmp(haystack + i, needle, nLen) == 0)
            return haystack + i;

        i = j + 1;
    }

    return nullptr;
}
