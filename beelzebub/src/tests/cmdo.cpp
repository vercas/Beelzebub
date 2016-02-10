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

#ifdef __BEELZEBUB__TEST_CMDO

#include <tests/cmdo.hpp>
#include <cmd_options.hpp>

#include <string.h>
#include <debug.hpp>

using namespace Beelzebub;

char OptionsString1[] = "   --alpha='test a' -b \"yada yada\" --g\"amm\"a ra\\da --si\\erra -x  ";
char OptionsString2[] = "--alpha='test a' -bx \"yada yada\" --g\"amm\"a ra\\da --si\\erra";
char OptionsString3[] = "-1 -0x060 -2 -987 -3 0123 -4 0b0011001010 --BT=On --BF No";
char OptionsString4[] = "--aaa=1 --bbb 2 -c 3 --ddd --blergh=yada";

CommandLineOptionParser parser;

#define PARSE_OPT(name) do {                                \
    res = parser.ParseOption(MCATS(CMDO_, name));           \
    ASSERT(res.IsOkayResult()                               \
        , "Failed to parse command-line option \"%s\": %H"  \
        , #name, res);                                      \
} while (false)

#define PARSE_OPT2(name, resVal) do {                       \
    res = parser.ParseOption(MCATS(CMDO_, name));           \
    ASSERT(res == resVal                                    \
        , "Failed to parse command-line option \"%s\": %H"  \
        , #name, res);                                      \
} while (false)

#define CHECK_STR(name, val) do {                                   \
    ASSERT(MCATS(CMDO_, name).StringValue != nullptr                \
        , "Command-line option \"%s\" should have had a non-null "  \
          "string value."                                           \
        , #name);                                                   \
                                                                    \
    ASSERT(strcmp(MCATS(CMDO_, name).StringValue, val) == 0         \
        , "Command-line option \"%s\" should have had value \"%s\"" \
          ", got \"%s\"."                                           \
        , #name, val, MCATS(CMDO_, name).StringValue);              \
} while (false)

#define CHECK_BOL(name, val) do {                                   \
    ASSERT(MCATS(CMDO_, name).BooleanValue == val                   \
        , "Command-line option \"%s\" should have had value %B"     \
          ", got %B."                                               \
        , #name, val, MCATS(CMDO_, name).BooleanValue);             \
} while (false)

#define CHECK_SINT(name, val) do {                                  \
    ASSERT(MCATS(CMDO_, name).SignedIntegerValue == val             \
        , "Command-line option \"%s\" should have had value %i8"    \
          ", got %i8."                                              \
        , #name, val, MCATS(CMDO_, name).SignedIntegerValue);       \
} while (false)

void TestCmdo()
{
    CMDO(1, "a", "alpha", String);
    CMDO(2, "b", "beta", String);
    CMDO(3, "c", "gamma", String);

    CMDO(4, "s", "sierra", BooleanByPresence);
    CMDO(5, "x", "xenulol", BooleanByPresence);
    CMDO(6, "y", "york", BooleanByPresence);

    Handle res;

    res = parser.StartParsing(OptionsString1);

    ASSERT(res.IsOkayResult()
        , "Failed to start parsing command-line options: %H"
        , res);

    // for (size_t i = 0; i < parser.Length; ++i)
    //     if (parser.InputString[i] == '\0')
    //         parser.InputString[i] = '_';

    // msg("Length of \"%S\" is: %us.%n", parser.Length, parser.InputString, parser.Length);

    ASSERT(parser.TokenCount == 7
        , "Parser should have identified %us tokens, not %us."
        , 7, parser.TokenCount);

    PARSE_OPT(1);
    PARSE_OPT(2);
    PARSE_OPT(3);
    PARSE_OPT(4);
    PARSE_OPT(5);
    PARSE_OPT(6);

    CHECK_STR(1, "test a");
    CHECK_STR(2, "yada yada");
    CHECK_STR(3, "rada");
    CHECK_BOL(4, true);
    CHECK_BOL(5, true);
    CHECK_BOL(6, false);

    //  AND NOW ROUND 2!

    new (&parser) CommandLineOptionParser();

    res = parser.StartParsing(OptionsString2);

    ASSERT(res.IsOkayResult()
        , "Failed to start parsing command-line options: %H"
        , res);

    // for (size_t i = 0; i < parser.Length; ++i)
    //     if (parser.InputString[i] == '\0')
    //         parser.InputString[i] = '_';

    // msg("Length of \"%S\" is: %us.%n", parser.Length, parser.InputString, parser.Length);

    ASSERT(parser.TokenCount == 6
        , "Parser should have identified %us tokens, not %us."
        , 6, parser.TokenCount);

    PARSE_OPT(1);
    PARSE_OPT(2);
    PARSE_OPT(3);
    PARSE_OPT(4);
    PARSE_OPT(5);
    PARSE_OPT(6);

    CHECK_STR(1, "test a");
    CHECK_STR(2, "yada yada");
    CHECK_STR(3, "rada");
    CHECK_BOL(4, true);
    CHECK_BOL(5, true);
    CHECK_BOL(6, false);

    //  AND NOW ROUND 3!

    CMDO(a, "1", "one", SignedInteger);
    CMDO(b, "2", "two", SignedInteger);
    CMDO(c, "3", "three", SignedInteger);
    CMDO(d, "4", "four", SignedInteger);
    CMDO(e, "BT", "BT", BooleanExplicit);
    CMDO(f, "BF", "BF", BooleanExplicit);

    new (&parser) CommandLineOptionParser();

    res = parser.StartParsing(OptionsString3);

    ASSERT(res.IsOkayResult()
        , "Failed to start parsing command-line options: %H"
        , res);

    // for (size_t i = 0; i < parser.Length; ++i)
    //     if (parser.InputString[i] == '\0')
    //         parser.InputString[i] = '_';

    // msg("Length of \"%S\" is: %us.%n", parser.Length, parser.InputString, parser.Length);

    ASSERT(parser.TokenCount == 11
        , "Parser should have identified %us tokens, not %us."
        , 11, parser.TokenCount);

    PARSE_OPT(a);
    PARSE_OPT(b);
    PARSE_OPT(c);
    PARSE_OPT(d);
    PARSE_OPT(e);
    PARSE_OPT(f);

    CHECK_SINT(a, -0x60);
    CHECK_SINT(b, -987);
    CHECK_SINT(c, 0123);
    CHECK_SINT(d, 0xCA);
    CHECK_BOL(e, true);
    CHECK_BOL(f, false);

    //  AND FINALLY ROUND 4!

    CMDO(01, "a", "aaa", String);
    CMDO_LINKED(02, "b", "bbb", String, 01);
    CMDO_LINKED(03, "c", "ccc", String, 02);
    CMDO_LINKED(04, "d", "ddd", BooleanByPresence, 03);
    CMDO_LINKED(05, "e", "eee", BooleanByPresence, 04);

    new (&parser) CommandLineOptionParser();

    res = parser.StartParsing(OptionsString4);

    ASSERT(res.IsOkayResult()
        , "Failed to start parsing command-line options: %H"
        , res);

    ASSERT(parser.TokenCount == 7
        , "Parser should have identified %us tokens, not %us."
        , 7, parser.TokenCount);

    CommandLineOptionBatchState batch;

    res = parser.StartBatch(batch, &CMDO_05);

    ASSERT(res.IsOkayResult()
        , "Failed to start batch for processing command-line options: %H"
        , res);
    //  This shan't ever happen.

    size_t counter = 0;

    while (!(res = batch.Next()).IsResult(HandleResult::UnsupportedOperation))
    {
        msg("?? %H ??%n", res);

        if (counter < 4)
        {
            ASSERT(res.IsOkayResult()
                , "Failed batch step of command-line options processing: %H"
                , res);
        }
        else
        {
            ASSERT(counter == 4
                , "Counter shouldn't exceed 4! It is %us."
                , counter);

            ASSERT(res.IsResult(HandleResult::NotFound)
                , "Failed batch step of command-line options processing: %H"
                , res);
        }

        ++counter;
    }

    CHECK_STR(01, "1");
    CHECK_STR(02, "2");
    CHECK_STR(03, "3");
    CHECK_BOL(04, true);
    CHECK_BOL(05, false);
}

#endif
