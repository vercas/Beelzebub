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
#include <metaprogramming.h>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

unsigned int System::EncodeJump(void * location, void * destination)
{
    uintptr_t const loc = reinterpret_cast<uintptr_t>(location),
        dst = reinterpret_cast<uintptr_t>(destination);

    ptrdiff_t const diff = dst - loc;
    //  Jumps are relative!

    if (diff == 0)
    {
        //  There's really nothing to jump to.

        return true;
    }
    else if (0 < diff && diff < 10)
    {
        //  This is the job for a no-op, not a relative jump.

        bool res = TurnIntoNoOp(location, destination, false);

        if (res)
            return diff;
        else
            return 0;
    }
    else if (-126 <= diff && diff <= 129)
    {
        //  Means a relative 8-bit offset can be used.

        msg("1-byte rel jump @ %Xp (%i1) ", loc, (int8_t)(diff - 2));

        *(reinterpret_cast<uint8_t *>(loc    )) = 0xEB;
        *(reinterpret_cast<int8_t *>(loc + 1)) = (int8_t)(diff - 2);
        //  The bottom byte of the difference will sign-extend to its value.

        return 2;
    }
    else if (-2147483643LL <= diff && diff <= 2147483652LL)
    {
        //  Means a relative 32-bit offset needs to be used.

        msg("4-byte rel jump @ %Xp (%i4) ", loc, (int32_t)(diff - 5));

        *(reinterpret_cast<uint8_t *>(loc    )) = 0xE9;
        *(reinterpret_cast<int32_t *>(loc + 1)) = (int32_t)(diff - 5);
        //  The bottom dword of the difference will sign-extend to its value.

        return 5;
    }
    else
    {
        //  Difference too large; cannot encode.

        return 0;
    }
}
