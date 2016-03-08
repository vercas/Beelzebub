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

#ifdef __BEELZEBUB__TEST_BIGINT

#include <utils/bigint.hpp>
#include <math.h>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

typedef BigUInt<15> BIT;

void TestBigInt()
{
    BIT a = 5ULL, b = 55ULL, c;

    c = a + b;

    ASSERT_EQ("%u4",   2U, c.CurrentSize);
    ASSERT_EQ("%u4",  60U, c.Data[0]);
    ASSERT_EQ("%u4",   0U, c.Data[1]);

    c += b;

    ASSERT_EQ("%u4",   2U, c.CurrentSize);
    ASSERT_EQ("%u4", 115U, c.Data[0]);
    ASSERT_EQ("%u4",   0U, c.Data[1]);

    BIT d = 0xFFFFFFFAULL;

    c += d;

    ASSERT_EQ("%u4",    2U, c.CurrentSize);
    ASSERT_EQ("%u4", 0x6DU, c.Data[0]);
    ASSERT_EQ("%u4",    1U, c.Data[1]);

    BIT e = 0xFFFFFFFFFFFFFFFFULL;

    c += e;

    ASSERT_EQ("%u4",    3U, c.CurrentSize);
    ASSERT_EQ("%u4", 0x6CU, c.Data[0]);
    ASSERT_EQ("%u4",    1U, c.Data[1]);
    ASSERT_EQ("%u4",    1U, c.Data[2]);

    c += c;

    ASSERT_EQ("%u4",    3U, c.CurrentSize);
    ASSERT_EQ("%u4", 0xD8U, c.Data[0]);
    ASSERT_EQ("%u4",    2U, c.Data[1]);
    ASSERT_EQ("%u4",    2U, c.Data[2]);

    c -= a;

    ASSERT_EQ("%u4",    3U, c.CurrentSize);
    ASSERT_EQ("%u4", 0xD3U, c.Data[0]);
    ASSERT_EQ("%u4",    2U, c.Data[1]);
    ASSERT_EQ("%u4",    2U, c.Data[2]);

    b -= a;

    ASSERT_EQ("%u4",  2U, b.CurrentSize);
    ASSERT_EQ("%u4", 50U, b.Data[0]);
    ASSERT_EQ("%u4",  0U, b.Data[1]);

    c -= e;

    ASSERT_EQ("%u4",    3U, c.CurrentSize);
    ASSERT_EQ("%u4", 0xD4U, c.Data[0]);
    ASSERT_EQ("%u4",    2U, c.Data[1]);
    ASSERT_EQ("%u4",    1U, c.Data[2]);

    d = a * b;

    ASSERT_EQ("%u4",   4U, d.CurrentSize);
    ASSERT_EQ("%u4", 250U, d.Data[0]);
    ASSERT_EQ("%u4",   0U, d.Data[1]);
    ASSERT_EQ("%u4",   0U, d.Data[2]);
    ASSERT_EQ("%u4",   0U, d.Data[3]);

    d.Data[2] = 3;

    BIT f = c, g = c * d;

    ASSERT_EQ("%u4",     7U, g.CurrentSize);
    ASSERT_EQ("%u4", 53000U, g.Data[0]);
    ASSERT_EQ("%u4",   500U, g.Data[1]);
    ASSERT_EQ("%u4",   886U, g.Data[2]);
    ASSERT_EQ("%u4",     6U, g.Data[3]);
    ASSERT_EQ("%u4",     3U, g.Data[4]);
    ASSERT_EQ("%u4",     0U, g.Data[5]);
    ASSERT_EQ("%u4",     0U, g.Data[6]);

    c *= d;

    ASSERT_EQ("%u4", 53000U, f.Data[0] * d.Data[0]);

    ASSERT_EQ("%u4",     7U, c.CurrentSize);
    ASSERT_EQ("%u4", 53000U, c.Data[0]);
    ASSERT_EQ("%u4",   500U, c.Data[1]);
    ASSERT_EQ("%u4",   886U, c.Data[2]);
    ASSERT_EQ("%u4",     6U, c.Data[3]);
    ASSERT_EQ("%u4",     3U, c.Data[4]);
    ASSERT_EQ("%u4",     0U, c.Data[5]);
    ASSERT_EQ("%u4",     0U, c.Data[6]);

    //  Now overflow propagation.

    ASSERT_EQ("%u4",          2U, e.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, e.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, e.Data[1]);

    f = e * e;

    ASSERT_EQ("%u4",          4U, f.CurrentSize);
    ASSERT_EQ("%X4", 0x00000001U, f.Data[0]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[1]);
    ASSERT_EQ("%X4", 0xFFFFFFFEU, f.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[3]);

    e *= e;

    ASSERT_EQ("%u4",          4U, e.CurrentSize);
    ASSERT_EQ("%X4", 0x00000001U, e.Data[0]);
    ASSERT_EQ("%X4", 0x00000000U, e.Data[1]);
    ASSERT_EQ("%X4", 0xFFFFFFFEU, e.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, e.Data[3]);

    e += b;

    ASSERT_EQ("%u4",          4U, e.CurrentSize);
    ASSERT_EQ("%X4", 0x00000033U, e.Data[0]);
    ASSERT_EQ("%X4", 0x00000000U, e.Data[1]);
    ASSERT_EQ("%X4", 0xFFFFFFFEU, e.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, e.Data[3]);

    f = e - b;

    ASSERT_EQ("%u4",          4U, f.CurrentSize);
    ASSERT_EQ("%X4", 0x00000001U, f.Data[0]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[1]);
    ASSERT_EQ("%X4", 0xFFFFFFFEU, f.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[3]);

    c += f;

    ASSERT_EQ("%u4",     7U, c.CurrentSize);
    ASSERT_EQ("%u4", 53001U, c.Data[0]);
    ASSERT_EQ("%u4",   500U, c.Data[1]);
    ASSERT_EQ("%u4",   884U, c.Data[2]);
    ASSERT_EQ("%u4",     6U, c.Data[3]);
    ASSERT_EQ("%u4",     4U, c.Data[4]);
    ASSERT_EQ("%u4",     0U, c.Data[5]);
    ASSERT_EQ("%u4",     0U, c.Data[6]);

    BIT h = 0xFFFFFFFFFFFFFFFFULL;

    f = f + h;

    ASSERT_EQ("%u4",          4U, f.CurrentSize);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[0]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[1]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[3]);

    f += h;

    ASSERT_EQ("%u4",          4U, f.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[1]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[3]);

    f += h;

    ASSERT_EQ("%u4",          5U, f.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFEU, f.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[1]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[2]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[3]);
    ASSERT_EQ("%X4", 0x00000001U, f.Data[4]);

    f = f - h;

    ASSERT_EQ("%u4",          5U, f.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[1]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[3]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[4]);

    f = f | h;  //  Should basically remain unchanged.

    ASSERT_EQ("%u4",          5U, f.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[1]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[3]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[4]);

    f |= h;     //  And again.

    ASSERT_EQ("%u4",          5U, f.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[1]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[3]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[4]);

    f &= h;     //  Mostly changed.

    ASSERT_EQ("%u4",          5U, f.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[1]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[2]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[3]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[4]);

    f = f & h;  //  And a size reduction.

    ASSERT_EQ("%u4",          2U, f.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[1]);

    f ^= h;     //  Nulling out.

    ASSERT_EQ("%u4",          2U, f.CurrentSize);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[0]);
    ASSERT_EQ("%X4", 0x00000000U, f.Data[1]);

    f = f ^ h;  //  And restoring.

    ASSERT_EQ("%u4",          2U, f.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, f.Data[1]);

    c = c ^ f;  //  Funky.

    ASSERT_EQ("%u4",          7U, c.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFF30F6U, c.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFE0BU, c.Data[1]);
    ASSERT_EQ("%u4",        884U, c.Data[2]);
    ASSERT_EQ("%u4",          6U, c.Data[3]);
    ASSERT_EQ("%u4",          4U, c.Data[4]);
    ASSERT_EQ("%u4",          0U, c.Data[5]);
    ASSERT_EQ("%u4",          0U, c.Data[6]);

    c |= f;     //  Less funky.

    ASSERT_EQ("%u4",          7U, c.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, c.Data[0]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, c.Data[1]);
    ASSERT_EQ("%u4",        884U, c.Data[2]);
    ASSERT_EQ("%u4",          6U, c.Data[3]);
    ASSERT_EQ("%u4",          4U, c.Data[4]);
    ASSERT_EQ("%u4",          0U, c.Data[5]);
    ASSERT_EQ("%u4",          0U, c.Data[6]);

    BIT i = ~c;

    ASSERT_EQ("%u4",          7U, i.CurrentSize);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[0]);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[1]);
    ASSERT_EQ("%X4", 0xFFFFFC8BU, i.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFFFF9U, i.Data[3]);
    ASSERT_EQ("%X4", 0xFFFFFFFBU, i.Data[4]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, i.Data[5]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, i.Data[6]);

    i <<= 36;

    ASSERT_EQ("%u4",          9U, i.CurrentSize);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[0]);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[1]);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[2]);
    ASSERT_EQ("%X4", 0xFFFFC8B0U, i.Data[3]);
    ASSERT_EQ("%X4", 0xFFFFFF9FU, i.Data[4]);
    ASSERT_EQ("%X4", 0xFFFFFFBFU, i.Data[5]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, i.Data[6]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, i.Data[7]);
    ASSERT_EQ("%X4", 0x0000000FU, i.Data[8]);

    i = i << 4;

    ASSERT_EQ("%u4",          9U, i.CurrentSize);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[0]);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[1]);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[2]);
    ASSERT_EQ("%X4", 0xFFFC8B00U, i.Data[3]);
    ASSERT_EQ("%X4", 0xFFFFF9FFU, i.Data[4]);
    ASSERT_EQ("%X4", 0xFFFFFBFFU, i.Data[5]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, i.Data[6]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, i.Data[7]);
    ASSERT_EQ("%X4", 0x000000FFU, i.Data[8]);

    i = i >> 36;

    ASSERT_EQ("%u4",          8U, i.CurrentSize);
    ASSERT_EQ("%X4", 0x0000000FU, i.Data[7]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, i.Data[6]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, i.Data[5]);
    ASSERT_EQ("%X4", 0xFFFFFFBFU, i.Data[4]);
    ASSERT_EQ("%X4", 0xFFFFFF9FU, i.Data[3]);
    ASSERT_EQ("%X4", 0xFFFFC8B0U, i.Data[2]);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[1]);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[0]);

    i >>= 4;

    ASSERT_EQ("%u4",          7U, i.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, i.Data[6]);
    ASSERT_EQ("%X4", 0xFFFFFFFFU, i.Data[5]);
    ASSERT_EQ("%X4", 0xFFFFFFFBU, i.Data[4]);
    ASSERT_EQ("%X4", 0xFFFFFFF9U, i.Data[3]);
    ASSERT_EQ("%X4", 0xFFFFFC8BU, i.Data[2]);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[1]);
    ASSERT_EQ("%X4", 0x00000000U, i.Data[0]);

    BIT j = i[4];

    ASSERT_EQ("%u4",          4U, j.CurrentSize);
    ASSERT_EQ("%X4", 0xFFFFFFF9U, j.Data[3]);
    ASSERT_EQ("%X4", 0xFFFFFC8BU, j.Data[2]);
    ASSERT_EQ("%X4", 0x00000000U, j.Data[1]);
    ASSERT_EQ("%X4", 0x00000000U, j.Data[0]);
}

#endif
