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

#include <math.h>

using namespace Beelzebub;

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

static uint8_t const LogTable[] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,

    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,

    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,

    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,

    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};
//  Vital note: Logarithm of 0 should be negative infinity... But this'll make it 0.

uint8_t Beelzebub::Log2_32(uint32_t val)
{
    if (val >= (1U << 16))
        if (val >= (1U << 24))
            return 24 + LogTable[val >> 24];
        else
            return 16 + LogTable[val >> 16];
    else
        if (val >= (1U << 8))
            return  8 + LogTable[val >>  8];
        else
            return      LogTable[val      ];
}

uint8_t Beelzebub::Log2_64(uint64_t val)
{
    if (val >= (1ULL << 32))
        if (val >= (1ULL << 48))
            if (val >= (1ULL << 56))
                return 56 + LogTable[val >> 56];
            else
                return 48 + LogTable[val >> 48];
        else
            if (val >= (1ULL << 40))
                return 40 + LogTable[val >> 40];
            else
                return 32 + LogTable[val >> 32];
    else
        if (val >= (1ULL << 16))
            if (val >= (1ULL << 24))
                return 24 + LogTable[val >> 24];
            else
                return 16 + LogTable[val >> 16];
        else
            if (val >= (1ULL << 8))
                return  8 + LogTable[val >>  8];
            else
                return      LogTable[val      ];
}

struct PointerAndSize Beelzebub::IntersectMemoryRanges(struct PointerAndSize a, struct PointerAndSize b)
{
    uintptr_t maxStart = Maximum(a.Start, b.Start);
    uintptr_t minEnd = Minimum(a.Start + a.Size, b.Start + b.Size);

    if (minEnd <= maxStart)
        return {0, 0};
    else
        return {maxStart, minEnd - maxStart};
}
