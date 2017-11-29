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

#include <system/code_patch.hpp>
#include <string.h>

using namespace Beelzebub;
using namespace Beelzebub::System;

static uint8_t const Nop1[] = {0x90};
static uint8_t const Nop2[] = {0x66, 0x90};
static uint8_t const Nop3[] = {0x0F, 0x1F, 0x00};
static uint8_t const Nop4[] = {0x0F, 0x1F, 0x40, 0x00};
static uint8_t const Nop5[] = {0x0F, 0x1F, 0x44, 0x00, 0x00};
static uint8_t const Nop6[] = {0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00};
static uint8_t const Nop7[] = {0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00};
static uint8_t const Nop8[] = {0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t const Nop9[] = {0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint16_t const Nop2Ex = 0x9066;
static uint32_t const Nop4Ex = 0x00401F0FU;
static uint64_t const Nop8Ex = 0x0000000000841F0FULL;

bool System::TurnIntoNoOp(void * start, void * end, bool useJump)
{
    uintptr_t loc = reinterpret_cast<uintptr_t>(start);
    uintptr_t const dst = reinterpret_cast<uintptr_t>(end);

    ptrdiff_t diff = dst - loc;

    if (diff == 0)
        return true;
    //  There's really nothing to no-op.

    if (useJump && diff > 9)
    {
        //  There's quite a leap and we jumps are allowed.

        unsigned int const size = EncodeJump(start, end);

        if (size == 0)
            return false;
        //  It failed.

        ::memset(reinterpret_cast<void *>(loc + size), 0xCC, diff - size);

        return true;
    }

    //  So no jump was used. Time to fill in with no-ops!

    for (/* nothing */; diff >= 9; diff -= 9, loc += 9)
    {
        *(reinterpret_cast<uint8_t *>(loc)) = Nop9[0];
        //  The lower byte.

        *(reinterpret_cast<uint64_t *>(loc + 1)) = Nop8Ex;
        //  And the upper 8 bytes.
    }

    if (diff > 0)
        switch ((diff - 1) & 0x7)
        {
            //  Here be 8 cases possible...

        case 7:
            *(reinterpret_cast<uint64_t *>(loc)) = Nop8Ex;
            return true;

        case 6:
            ::memcpy(reinterpret_cast<void *>(loc), Nop7, 7);
            return true;

        case 5:
            ::memcpy(reinterpret_cast<void *>(loc), Nop6, 6);
            return true;

        case 4:
            ::memcpy(reinterpret_cast<void *>(loc), Nop5, 5);
            return true;

        case 3:
            *(reinterpret_cast<uint32_t *>(loc)) = Nop4Ex;
            return true;

        case 2:
            ::memcpy(reinterpret_cast<void *>(loc), Nop3, 3);
            return true;

        case 1:
            *(reinterpret_cast<uint16_t *>(loc)) = Nop2Ex;
            return true;

        case 0:
            *(reinterpret_cast<uint8_t *>(loc)) = Nop1[0];
            return true;
        }

    return true;
}
