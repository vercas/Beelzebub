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

#ifdef __BEELZEBUB__TEST_STR

#include <tests/string.hpp>
#include <string.h>
    
#include <debug.hpp>

using namespace Beelzebub;

char const * const tStrA = "";
char const * const tStrB = "a";
char const * const tStrC = "ab";
char const * const tStrD = "abc";
char const * const tStrE = "A";
char const * const tStrF = "AB";
char const * const tStrG = "ABC";
char const * const tStrH = "0000000000000000000000000000";
char const * const tStrI = "ABC";
char const * const tStrJ = "";

size_t const tStrAlen = 0;
size_t const tStrBlen = 1;
size_t const tStrClen = 2;
size_t const tStrDlen = 3;
size_t const tStrElen = 1;
size_t const tStrFlen = 2;
size_t const tStrGlen = 3;
size_t const tStrHlen = 28;
size_t const tStrIlen = 3;
size_t const tStrJlen = 0;

Handle TestStringLibrary()
{
#define testlen(name)                                        \
size_t MCATS(str, name, len) = strlen(MCATS(tStr, name));    \
ASSERT(MCATS(str, name, len) == MCATS(tStr, name, len)       \
    , "Failed string length test for string %s (\"%s\"): "   \
      "got %us, expected %us!"                               \
    , #name, MCATS(tStr, name), MCATS(str, name, len)        \
    , MCATS(tStr, name, len));

#define testNlen(tseq, name, nl, expctd)                                \
size_t MCATS(str, name, Nlen, tseq) = strnlen(MCATS(tStr, name), (nl)); \
ASSERT(MCATS(str, name, Nlen, tseq) == (size_t)(expctd)                 \
    , "Failed string capped length test for string %s (\"%s\"): "       \
      "got %us, expected %us (cap: %us; test sequence %us)!"            \
    , #name, MCATS(tStr, name), MCATS(str, name, Nlen, tseq)            \
    , (size_t)(expctd), (size_t)(nl), (size_t)(tseq));

    testlen(A);
    testlen(B);
    testlen(C);
    testlen(D);
    testlen(E);
    testlen(F);
    testlen(G);
    testlen(H);
    testlen(I);
    testlen(J);

    testNlen(1, A, 0, 0);
    testNlen(1, B, 0, 0);
    testNlen(1, C, 0, 0);
    testNlen(1, D, 0, 0);
    testNlen(1, E, 0, 0);
    testNlen(1, F, 0, 0);
    testNlen(1, G, 0, 0);
    testNlen(1, H, 0, 0);
    testNlen(1, I, 0, 0);
    testNlen(1, J, 0, 0);

    testNlen(2, A, 1, 0);
    testNlen(2, B, 1, 1);
    testNlen(2, C, 1, 1);
    testNlen(2, D, 1, 1);
    testNlen(2, E, 1, 1);
    testNlen(2, F, 1, 1);
    testNlen(2, G, 1, 1);
    testNlen(2, H, 1, 1);
    testNlen(2, I, 1, 1);
    testNlen(2, J, 1, 0);

    testNlen(3, A, 2, 0);
    testNlen(3, B, 2, 1);
    testNlen(3, C, 2, 2);
    testNlen(3, D, 2, 2);
    testNlen(3, E, 2, 1);
    testNlen(3, F, 2, 2);
    testNlen(3, G, 2, 2);
    testNlen(3, H, 2, 2);
    testNlen(3, I, 2, 2);
    testNlen(3, J, 2, 0);

    testNlen(4, A, 3, 0);
    testNlen(4, B, 3, 1);
    testNlen(4, C, 3, 2);
    testNlen(4, D, 3, 3);
    testNlen(4, E, 3, 1);
    testNlen(4, F, 3, 2);
    testNlen(4, G, 3, 3);
    testNlen(4, H, 3, 3);
    testNlen(4, I, 3, 3);
    testNlen(4, J, 3, 0);

    testNlen(5, A, 100, tStrAlen);
    testNlen(5, B, 100, tStrBlen);
    testNlen(5, C, 100, tStrClen);
    testNlen(5, D, 100, tStrDlen);
    testNlen(5, E, 100, tStrElen);
    testNlen(5, F, 100, tStrFlen);
    testNlen(5, G, 100, tStrGlen);
    testNlen(5, H, 100, tStrHlen);
    testNlen(5, I, 100, tStrIlen);
    testNlen(5, J, 100, tStrJlen);


    char const * strres = strstr(tStrD, tStrE);

    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);

    strres = strstr(tStrD, tStrB);
    ASSERT(strres == tStrD, "Expected %Xp, got %Xp.", tStrD, strres);

    strres = strstr(tStrD, tStrC);
    ASSERT(strres == tStrD, "Expected %Xp, got %Xp.", tStrD, strres);

    strres = strstr(tStrD, tStrD);
    ASSERT(strres == tStrD, "Expected %Xp, got %Xp.", tStrD, strres);


    strres = strstr(tStrG, tStrB);
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);

    strres = strstr(tStrG, tStrE);
    ASSERT(strres == tStrG, "Expected %Xp, got %Xp.", tStrG, strres);

    strres = strstr(tStrG, tStrF);
    ASSERT(strres == tStrG, "Expected %Xp, got %Xp.", tStrG, strres);

    strres = strstr(tStrG, tStrG);
    ASSERT(strres == tStrG, "Expected %Xp, got %Xp.", tStrG, strres);


    strres = strstr(tStrG, "BC");
    ASSERT(strres == tStrG + 1, "Expected %Xp, got %Xp.", tStrG + 1, strres);

    strres = strstr(tStrG, "C");
    ASSERT(strres == tStrG + 2, "Expected %Xp, got %Xp.", tStrG + 2, strres);


    strres = strstr(tStrG, tStrH);
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);


    char const * blahA = "a", * blahB = "bb", * blahC = "ccc";
    char const * yadaA = "a,b", * yadaB = "bb,cd;ef", * yadaC = "ccc,def;ghi.jkl";


    strres = strstrex(blahA, "a", ",;");
    ASSERT(strres == blahA, "Expected %Xp, got %Xp.", blahA, strres);

    strres = strstrex(blahB, "a", ",;");
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);

    strres = strstrex(blahC, "a", ",;");
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);


    strres = strstrex(blahB, "bb", ",;");
    ASSERT(strres == blahB, "Expected %Xp, got %Xp.", blahB, strres);

    strres = strstrex(yadaB, "bb", ",;");
    ASSERT(strres == yadaB, "Expected %Xp, got %Xp.", yadaB, strres);


    strres = strstrex(blahC, "ccc", ",;");
    ASSERT(strres == blahC, "Expected %Xp, got %Xp.", blahC, strres);

    strres = strstrex(yadaC, "ccc", ",;");
    ASSERT(strres == yadaC, "Expected %Xp, got %Xp.", yadaC, strres);


    strres = strstrex(blahA, "b", ",;");
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);

    strres = strstrex(blahB, "b", ",;");
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);

    strres = strstrex(blahC, "b", ",;");
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);


    strres = strstrex(yadaA, "a", ",;");
    ASSERT(strres == yadaA, "Expected %Xp, got %Xp.", yadaA, strres);

    strres = strstrex(yadaB, "a", ",;");
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);

    strres = strstrex(yadaC, "a", ",;");
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);


    strres = strstrex(yadaA, "b", ",;");
    ASSERT(strres == yadaA + 2, "Expected %Xp, got %Xp.", yadaA + 2, strres);

    strres = strstrex(yadaB, "b", ",;");
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);

    strres = strstrex(yadaC, "b", ",;");
    ASSERT(strres == nullptr, "Expected null, got %Xp.", strres);


    strres = strstrex(yadaB, "cd", ",;");
    ASSERT(strres == yadaB + 3, "Expected %Xp, got %Xp.", yadaB + 3, strres);

    strres = strstrex(yadaB, "ef", ",;");
    ASSERT(strres == yadaB + 6, "Expected %Xp, got %Xp.", yadaB + 6, strres);


    strres = strstrex(yadaB, "bb,cd", ";");
    ASSERT(strres == yadaB, "Expected %Xp, got %Xp.", yadaB, strres);

    strres = strstrex(yadaB, "ef", ";");
    ASSERT(strres == yadaB + 6, "Expected %Xp, got %Xp.", yadaB + 6, strres);


    strres = strstrex(yadaB, "bb", ",");
    ASSERT(strres == yadaB, "Expected %Xp, got %Xp.", yadaB, strres);

    strres = strstrex(yadaB, "cd;ef", ",");
    ASSERT(strres == yadaB + 3, "Expected %Xp, got %Xp.", yadaB + 3, strres);


    strres = strstrex(yadaB, "cd", ",;");
    ASSERT(strres == yadaB + 3, "Expected %Xp, got %Xp.", yadaB + 3, strres);

    strres = strstrex(yadaB, "ef", ",;");
    ASSERT(strres == yadaB + 6, "Expected %Xp, got %Xp.", yadaB + 6, strres);


    strres = strstrex(yadaC, "ccc", ".,;");
    ASSERT(strres == yadaC, "Expected %Xp, got %Xp.", yadaC, strres);

    strres = strstrex(yadaC, "def", ".,;");
    ASSERT(strres == yadaC + 4, "Expected %Xp, got %Xp.", yadaC + 4, strres);

    strres = strstrex(yadaC, "ghi", ".,;");
    ASSERT(strres == yadaC + 8, "Expected %Xp, got %Xp.", yadaC + 8, strres);

    strres = strstrex(yadaC, "jkl", ".,;");
    ASSERT(strres == yadaC + 12, "Expected %Xp, got %Xp.", yadaC + 12, strres);


    char const * testArgs = "mt,app";
    strres = strstrex(testArgs, "app", ",;");
    ASSERT(strres == testArgs + 3, "Expected %Xp, got %Xp.", testArgs + 3, strres);


#define testCaseCmpEq(strA, strB)                                           \
    ASSERT(strcasecmp(strA, strB) == 0                                      \
        , "Strings \"%s\" and \"%s\" should be case-insensitively equal."   \
          " Result is %i4."                                                 \
        , strA, strB, strcasecmp(strA, strB));

#define testCaseCmpEqN(strA, strB, len)                                     \
    ASSERT(strcasencmp(strA, strB, len) == 0                                \
        , "Strings \"%s\" and \"%s\" should be case-insensitively equal."   \
          " Result is %i4."                                                 \
        , strA, strB, strcasecmp(strA, strB));

    testCaseCmpEq("rada", "rAdA");
    testCaseCmpEqN("rada", "rAdA", 5);


    return HandleResult::Okay;
}

#endif
