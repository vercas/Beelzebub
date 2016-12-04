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

#include <utils/unit_tests.hpp>
#include <beel/exceptions.hpp>

#include <terminals/cache.hpp>
#include <terminals/cache_pools_heap.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::Utils;

/*****************************
    UnitTestSection struct
*****************************/

static UnitTestSection const * currentSection = nullptr;

/*  Constructors  */

UnitTestSection::UnitTestSection(char const * const name)
    : Name(name), Next(currentSection)
{
    currentSection = this;
}

UnitTestSection::UnitTestSection(char const * const name
                    , UnitTestSection const * const next)
    : Name(name), Next(next)
{
    currentSection = this;
}

/*  Destructor  */

UnitTestSection::~UnitTestSection()
{
    currentSection = this->Next;
}

/****************************
    Other unit test stuff
****************************/

__extern uintptr_t tests_data_section_start;
__extern uintptr_t tests_data_section_end;

TerminalBase * Beelzebub::Utils::UnitTestTerminal = nullptr;

#ifndef __BEELZEBUB_SETTINGS_UNIT_TESTS_QUIET
static CacheTerminal cacheTerminal;
#endif

static UnitTestDeclaration * currentTest;

static __unit_test_startup void PrintSection(UnitTestSection const * const section)
{
    if likely(section != nullptr)
    {
        PrintSection(section->Next);
        //  Aye, 'last' (top) section is printed first.

        //  DebugTerminal is guaranteed to be non-null.

        DEBUG_TERM << " -> " << section->Name;
    }
}

UnitTestsReport Beelzebub::Utils::RunUnitTests()
{
    UnitTestDeclaration * last = nullptr;

    for (uintptr_t * prol = &tests_data_section_start; prol < &tests_data_section_end; )
    {
        if (*prol != UnitTestDeclaration::PrologueValue)
        {
            //  Not a unit test declaration's start.

            ++prol;
            continue;
        }

        UnitTestDeclaration * decl = reinterpret_cast<UnitTestDeclaration *>(prol);

        if (decl->Epilogue != UnitTestDeclaration::EpilogueValue)
        {
            //  Odd that the declaration starts well, but doesn't end correctly...

            ++prol;
            continue;
        }

        //  This appears to be a valid test declaration. Sadly, they cannot be
        //  checksummed or hashed because of symbols & linking & stuff...

        decl->Next = last;
        decl->Status = UnitTestStatus::Awaiting;

        prol = reinterpret_cast<uintptr_t *>(decl + 1);
        last = decl;
    }

#ifndef __BEELZEBUB_SETTINGS_UNIT_TESTS_QUIET
    new (&cacheTerminal) CacheTerminal(&AcquireCharPoolInKernelHeap
                                     , &EnlargeCharPoolInKernelHeap
                                     , &ReleaseCharPoolFromKernelHeap);

    UnitTestTerminal = &cacheTerminal;
#else
    UnitTestTerminal = nullptr;
#endif

    size_t countAll = 0, countSucceded = 0, countFailed = 0;
    UnitTestDeclaration * first = last; //  Makes little sense, but oh well.

    for (/* nothing */; last != nullptr; last = last->Next)
    {
        last->Status = UnitTestStatus::Running;
        ++countAll;

        currentTest = last;

        currentSection = nullptr;
        //  The current section needs to be reset, because a failing test may
        //  have left a chain of sections.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

        __try
        {
            last->Function();

            last->Status = UnitTestStatus::Succeeded;
            ++countSucceded;

            if (Debug::DebugTerminal != nullptr)
            {
                DEBUG_TERM << "[UNIT TEST] (" << "SUCCESS" << ") [SUITE] "
                    << (currentTest->Suite == nullptr ? "% NULL %" : currentTest->Suite);

                if (currentTest->Case != nullptr)
                    DEBUG_TERM << " [CASE] " << currentTest->Case;

                DEBUG_TERM << EndLine;
            }
        }
        __catch (x)
        {
            last->Status = UnitTestStatus::Failed;
            ++countFailed;
        }

#pragma GCC diagnostic pop

#ifndef __BEELZEBUB_SETTINGS_UNIT_TESTS_QUIET
        cacheTerminal.Clear();
#endif
    }

#ifndef __BEELZEBUB_SETTINGS_UNIT_TESTS_QUIET
    UnitTestTerminal = nullptr;
#endif

    if (Debug::DebugTerminal != nullptr)
        DEBUG_TERM  << "[UNIT TEST] (REPORT) " << countAll
                    << " tests, " << countSucceded << " succeeded, " << countFailed
                    << " failed." << EndLine;

#ifndef __BEELZEBUB_SETTINGS_UNIT_TESTS_QUIET
    Handle res = cacheTerminal.Destroy();

    assert(res.IsOkayResult()
        , "Failed to destroy unit test cache terminal: %H%n"
        , res);
#endif

    return {countAll, countSucceded, first};
}

void Beelzebub::Utils::FailUnitTest(char const * const fileName, int const line)
{
    Exception * x = GetException();

    x->Type = ExceptionType::UnitTestFailure;
    x->UnitTestFailure.FileName = fileName;
    x->UnitTestFailure.Line = line;

    if (Debug::DebugTerminal != nullptr)
    {
        DEBUG_TERM << "[UNIT TEST] (" << "FAIL" << ") [SUITE] "
            << (currentTest->Suite == nullptr ? "% NULL %" : currentTest->Suite);

        if (currentTest->Case != nullptr)
            DEBUG_TERM << " [CASE] " << currentTest->Case;

        PrintSection(currentSection);

        DEBUG_TERM << EndLine << fileName << ":" << line << EndLine;
    }

#ifndef __BEELZEBUB_SETTINGS_UNIT_TESTS_QUIET
    if (Debug::DebugTerminal != nullptr)
        DEBUG_TERM  << "################################## INFO START ##################################"
                    << EndLine << cacheTerminal << EndLine
                    << "################################### INFO END ###################################"
                    << EndLine;
#endif

    ThrowException();

    //  Reaching this point means there is no exception context to catch this
    //  exception.

    FAIL("Unit test failure in:%n%s:%i4%nThere was no exception context available."
        , fileName, line);
}
