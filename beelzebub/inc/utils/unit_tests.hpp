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

#pragma once

#include <metaprogramming.h>

#define __test_function static __used __section(text.tests) void 

#define __test_declaration(dName, ...) \
    static __used __section(data.tests) UnitTestDeclaration dName {__VA_ARGS__}

#define DEFINE_TEST(tSuite, tCase) \
    __test_function MCATS(__test_function_, __LINE__)(); \
    __test_declaration(MCATS(__test_declaration_, __LINE__), &(MCATS(__test_function_, __LINE__)), tSuite, tCase); \
    void MCATS(__test_function_, __LINE__)()

namespace Beelzebub { namespace Utils
{
    typedef void (* TestFunction)(void);

    enum class UnitTestStatus : uintptr_t
    {
        Unknown     = 0,
        Awaiting    = 1,
        Running     = 2,
        Succeeded   = 3,
        Failed      = 4,
    };

    struct UnitTestDeclaration
    {
        /*  Statics  */

        static constexpr uintptr_t const PrologueValue = 0x010102;
        static constexpr uintptr_t const EpilogueValue = 0x909010;
        //  Geddit?

        /*  Constructors  */

        inline constexpr UnitTestDeclaration(TestFunction const func
                                           , char const * const suite
                                           , char const * const kase)
            : Prologue(PrologueValue)
            , Status(UnitTestStatus::Unknown)
            , Function(func)
            , Suite(suite)
            , Case(kase)
            , Next(nullptr)
            , Epilogue(EpilogueValue)
        {

        }

        /*  Fields  */

        uintptr_t const Prologue;

        UnitTestStatus Status;
        TestFunction const Function;

        char const * const Suite;
        char const * const Case;

        UnitTestDeclaration * Next;

        uintptr_t const Epilogue;
    } __packed __aligned(8);

    __startup void RunUnitTests();

    __startup __noreturn void FailUnitTest(char const * const fileName
                                                 , int const line);

    __startup void UnitTestMessage(char const * fmt, ...);
}}

#define REQUIRE_1(cond) do {                                            \
if unlikely(!(cond))                                                    \
    Beelzebub::Utils::FailUnitTest(__FILE__, __LINE__);                 \
} while (false)

#define REQUIRE_N(cond, ...) do {                                       \
if unlikely(!(cond))                                                    \
    Beelzebub::Utils::UnitTestMessage(__VA_ARGS__);                     \
    Beelzebub::Utils::FailUnitTest(__FILE__, __LINE__);                 \
} while (false)

#define REQUIRE(...) GET_MACRO100(__VA_ARGS__, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, \
REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_N, REQUIRE_1)(__VA_ARGS__)
