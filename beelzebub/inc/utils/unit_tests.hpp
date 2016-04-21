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

#include <terminals/base.hpp>

#ifdef __BEELZEBUB_SETTINGS_UNIT_TESTS
    #define __unit_test_declaration __used __section(text.tests) 

    #define __DECLARE_TEST(dName, ...) \
        static __used __section(data.tests) UnitTestDeclaration dName {__VA_ARGS__}

    #define DEFINE_TEST_2(tSuite, tCase) \
        static __unit_test_declaration void MCATS(__test_function_, __LINE__)(); \
        __DECLARE_TEST(MCATS(__test_declaration_, __LINE__), &(MCATS(__test_function_, __LINE__)), #tSuite, #tCase); \
        void MCATS(__test_function_, __LINE__)()

    #define DEFINE_TEST_1(tSuite) \
        static __unit_test_declaration void MCATS(__test_function_, __LINE__)(); \
        __DECLARE_TEST(MCATS(__test_declaration_, __LINE__), &(MCATS(__test_function_, __LINE__)), #tSuite, nullptr); \
        void MCATS(__test_function_, __LINE__)()

    #define __unit_test_startup __startup
#else
    #define __unit_test_declaration __unused __section(text.tests) 

    #define DEFINE_TEST_2(tSuite, tCase) \
        static __unit_test_declaration void MCATS(__test_function_, __LINE__)()

    #define DEFINE_TEST_1(tSuite) \
        static __unit_test_declaration void MCATS(__test_function_, __LINE__)()

    #define __unit_test_startup __startup __unused
#endif

#define DEFINE_TEST(...) GET_MACRO2(__VA_ARGS__, DEFINE_TEST_2, DEFINE_TEST_1)(__VA_ARGS__)

#define SECTION(name) with (Beelzebub::Utils::UnitTestSection MCATS(__unit_test_section_, __LINE__) {#name})

namespace Beelzebub { namespace Utils
{
    extern Terminals::TerminalBase * UnitTestTerminal;

    typedef void (* TestFunction)(void);

    struct UnitTestSection
    {
        /*  Constructors  */

        __unit_test_startup UnitTestSection(char const * const name);
        __unit_test_startup UnitTestSection(char const * const name
                               , UnitTestSection const * const next);

        /*  Destructor  */

        __unit_test_startup ~UnitTestSection();
        
        /*  Fields  */

        char const * const Name;
        UnitTestSection const * const Next;
    };

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

    struct UnitTestsReport
    {
        size_t TestCount, SuccessCount;

        UnitTestDeclaration * FirstTest;
    };

    __unit_test_startup UnitTestsReport RunUnitTests();

    __unit_test_startup __noreturn void FailUnitTest(char const * const fileName
                                                   , int const line);
}}


#define INFO(...) do { \
    if likely(Beelzebub::Utils::UnitTestTerminal != nullptr) \
        Beelzebub::Utils::UnitTestTerminal->WriteFormat(__VA_ARGS__); \
} while (false)

#define INFO_(...) do { \
    if likely(Beelzebub::Utils::UnitTestTerminal != nullptr) \
        (*(Beelzebub::Utils::UnitTestTerminal) << __VA_ARGS__); \
} while (false)


#define REQUIRE_1(cond) do { \
    if unlikely(!(cond)) \
        Beelzebub::Utils::FailUnitTest(__FILE__, __LINE__); \
} while (false)

#define REQUIRE_N(cond, ...) do { \
    if unlikely(!(cond)) \
    { \
        if likely(Beelzebub::Utils::UnitTestTerminal != nullptr) \
            Beelzebub::Utils::UnitTestTerminal->WriteFormat(__VA_ARGS__); \
 \
        Beelzebub::Utils::FailUnitTest(__FILE__, __LINE__); \
    } \
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


#define REQUIRE_EQ(expected, val) do { \
if unlikely((val) != (expected)) { \
    if likely(Beelzebub::Utils::UnitTestTerminal != nullptr) \
        *(Beelzebub::Utils::UnitTestTerminal) << "Expected " \
            << expected << ", got " << val << "." \
            << Beelzebub::Terminals::EndLine; \
 \
    Beelzebub::Utils::FailUnitTest(__FILE__, __LINE__); \
} } while (false)


#define REQUIRE_NE(expected, val) do { \
if unlikely((val) == (expected)) { \
    if likely(Beelzebub::Utils::UnitTestTerminal != nullptr) \
        *(Beelzebub::Utils::UnitTestTerminal) << "Expected anything but " \
            << expected << "!" << Beelzebub::Terminals::EndLine; \
 \
    Beelzebub::Utils::FailUnitTest(__FILE__, __LINE__); \
} } while (false)


#define REQUIRE_LT(expected, val) do { \
if unlikely((val) >= (expected)) { \
    if likely(Beelzebub::Utils::UnitTestTerminal != nullptr) \
        *(Beelzebub::Utils::UnitTestTerminal) << "Expected less than " \
            << expected << ", got " << val << "." \
            << Beelzebub::Terminals::EndLine; \
 \
    Beelzebub::Utils::FailUnitTest(__FILE__, __LINE__); \
} } while (false)


#define REQUIRE_LE(expected, val) do { \
if unlikely((val) > (expected)) { \
    if likely(Beelzebub::Utils::UnitTestTerminal != nullptr) \
        *(Beelzebub::Utils::UnitTestTerminal) << "Expected less than on requal to " \
            << expected << ", got " << val << "." \
            << Beelzebub::Terminals::EndLine; \
 \
    Beelzebub::Utils::FailUnitTest(__FILE__, __LINE__); \
} } while (false)


#define REQUIRE_GT(expected, val) do { \
if unlikely((val) <= (expected)) { \
    if likely(Beelzebub::Utils::UnitTestTerminal != nullptr) \
        *(Beelzebub::Utils::UnitTestTerminal) << "Expected greater than " \
            << expected << ", got " << val << "." \
            << Beelzebub::Terminals::EndLine; \
 \
    Beelzebub::Utils::FailUnitTest(__FILE__, __LINE__); \
} } while (false)


#define REQUIRE_GE(expected, val) do { \
if unlikely((val) < (expected)) { \
    if likely(Beelzebub::Utils::UnitTestTerminal != nullptr) \
        *(Beelzebub::Utils::UnitTestTerminal) << "Expected greater than or equal to " \
            << expected << ", got " << val << "." \
            << Beelzebub::Terminals::EndLine; \
 \
    Beelzebub::Utils::FailUnitTest(__FILE__, __LINE__); \
} } while (false)
