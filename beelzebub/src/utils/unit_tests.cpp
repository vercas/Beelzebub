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
#include <exceptions.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

__extern uintptr_t tests_data_section_start;
__extern uintptr_t tests_data_section_end;

void Beelzebub::Utils::RunUnitTests()
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

    size_t countAll = 0, countSucceded = 0, countFailed = 0;

    for (/* nothing */; last != nullptr; last = last->Next)
    {
        last->Status = UnitTestStatus::Running;
        ++countAll;

        bool success = false;

        __try
        {
            last->Function();

            last->Status = UnitTestStatus::Succeeded;
            ++countSucceded;

            success = true;
        }
        __catch (x)
        {
            last->Status = UnitTestStatus::Failed;
            ++countFailed;

            success = false;
        }

        msg("[UNIT TEST] (%s) %s -> %s%n"
            , success ? "OKAY" : "FAIL"
            , last->Suite, last->Case);
    }
}

void Beelzebub::Utils::FailUnitTest(char const * const fileName, int const line)
{
    Exception * x = GetException();

    x->Type = ExceptionType::UnitTestFailure;
    x->UnitTestFailure.FileName = fileName;
    x->UnitTestFailure.Line = line;

    ThrowException();

    //  Reaching this point means there is no exception context to catch this
    //  exception.

    ASSERT(false
        , "Unit test failure in:%n%s:%i4%nThere was no exception context available."
        , fileName, line);
}

void Beelzebub::Utils::UnitTestMessage(char const * fmt, ...)
{
    //  TODO: Implement buffering and only display the info on failure!

    if (Debug::DebugTerminal == nullptr)
        return;
    //  I do the check here to avoid messing with the varargs.

    va_list args;

    va_start(args, fmt);
    Debug::DebugTerminal->Write(fmt, args);
    va_end(args);
}
