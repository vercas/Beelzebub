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

#ifdef __BEELZEBUB__TEST_TERMINAL

#include <tests/terminal.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

#define EXPECT(val)                 \
ASSERT(res.Size == (val)            \
    , "Expected size %u4, got %u4." \
    , (val), res.Size)

TerminalWriteResult TestTerminal()
{
    TerminalBase * term = Debug::DebugTerminal;

    TerminalWriteResult res {};
    uint32_t cnt = 0U;

    TERMTRY1(term->Write("rada rada"), res, cnt);
    EXPECT(9);

    TERMTRY1(term->Write("rada?"), res, cnt);
    EXPECT(14);

    TERMTRY1(term->WriteLine(" ...blergh?"), res, cnt);
    EXPECT(27);

    TERMTRY1(term->WriteFormat("%n", "yada"), res, cnt);
    EXPECT(29);

    TERMTRY1(term->WriteFormat("%s", "yada"), res, cnt);
    EXPECT(33);

    TERMTRY1(term->WriteFormat("%S", 4, "blah"), res, cnt);
    EXPECT(37);

    size_t mark1, mark2;

    TERMTRY1(term->WriteFormat("%#testing marks%#", &mark1, &mark2), res, cnt);
    EXPECT(50);

    ASSERT(mark1 ==  0, "Mark 1 should be 0, not %us.", mark1);
    ASSERT(mark2 == 13, "Mark 2 should be 13, not %us.", mark2);

    TERMTRY1(term->WriteFormat("%s%# further%#", "testing", &mark1, &mark2), res, cnt);
    EXPECT(65);

    ASSERT(mark1 ==  7, "Mark 1 should be 7, not %us.", mark1);
    ASSERT(mark2 == 15, "Mark 2 should be 15, not %us.", mark2);

    TERMTRY1(term->WriteLine(), res, cnt);
    EXPECT(67);

    TERMTRY1(term->WriteFormat("%s%#{%c; Key = %i4; Value = %i4}%#"
        , "t1"
        , &mark1
        , 'C'
        , 123
        , 456
        , &mark2), res, cnt);
    EXPECT(96);

    ASSERT(mark1 ==  2, "Mark 1 should be 2, not %us.", mark1);
    ASSERT(mark2 == 29, "Mark 2 should be 29, not %us.", mark2);

    TERMTRY1(term->Write("└─"), res, cnt);
    EXPECT(98);

    TERMTRY1(term->Write("├─"), res, cnt);
    EXPECT(100);

    TERMTRY1(term->Write("┐"), res, cnt);
    EXPECT(101);

    TERMTRY1(term->Write("│"), res, cnt);
    EXPECT(102);

    TERMTRY1(term->WriteFormat("%s", "└─"), res, cnt);
    EXPECT(104);

    TERMTRY1(term->WriteFormat("└─"), res, cnt);
    EXPECT(106);

    TERMTRY1(term->WriteFormat("├─"), res, cnt);
    EXPECT(108);

    TERMTRY1(term->WriteFormat("%#├─%#", &mark1, &mark2), res, cnt);
    EXPECT(110);

    ASSERT(mark1 == 0, "Mark 1 should be 0, not %us.", mark1);
    ASSERT(mark2 == 2, "Mark 2 should be 2, not %us.", mark2);

    TERMTRY1(term->WriteFormat("%#%s{123456}%#", &mark1, "├─", &mark2), res, cnt);
    EXPECT(120);

    ASSERT(mark1 ==  0, "Mark 1 should be 0, not %us.", mark1);
    ASSERT(mark2 == 10, "Mark 2 should be 10, not %us.", mark2);

    TERMTRY1(term->WriteFormat("%s%#{123456}%#", "├─", &mark1, &mark2), res, cnt);
    EXPECT(130);

    ASSERT(mark1 ==  2, "Mark 1 should be 2, not %us.", mark1);
    ASSERT(mark2 == 10, "Mark 2 should be 10, not %us.", mark2);

    TERMTRY1(term->WriteFormat("%#%*%#", &mark1, (size_t)30, &mark2), res, cnt);
    EXPECT(160);

    ASSERT(mark1 ==  0, "Mark 1 should be 0, not %us.", mark1);
    ASSERT(mark2 == 30, "Mark 2 should be 30, not %us.", mark2);

    TERMTRY1(term->WriteLine(), res, cnt);

    *term << 123 << " equals " << 123ULL << EndLine
          << "This should be on another line." << EndLine
          << HandleResult::NotImplemented << EndLine
          << Hexadecimal << (short)12345 << Decimal << EndLine
          << ShowPlus << -1 << ' ' << 1 << HidePlus << EndLine
          << NumericLowercase << Hexadecimal << (unsigned char)0xAB << Decimal << NumericUppercase << EndLine;

    return res;
}

#endif
