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

#include <debug.hpp>
#include <crt0.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::Terminals;

TerminalBase * Debug::DebugTerminal;

TerminalBase * Debug::GetDebugTerminal()
{
    return DebugTerminal;
}

void Debug::CatchFire(char const * const file
                    , size_t const line
                    , char const * const cond
                    , char const * const msg)
{
    if (DebugTerminal != nullptr && DebugTerminal->Capabilities->CanOutput)
    {
        DebugTerminal->WriteLine("");
        DebugTerminal->Write("CAUGHT FIRE at line ");
        DebugTerminal->WriteUIntD(line);
        DebugTerminal->Write(" of \"");
        DebugTerminal->Write(file);

        if (msg == nullptr)
            DebugTerminal->WriteLine("\".");
        else
        {
            DebugTerminal->WriteLine("\":");
            DebugTerminal->WriteLine(msg);
        }
    }

    QuitProcess(HandleResult::ImmediateTermination, -1);

    //  Spin when things go haywire.
    while (true) ;
    __unreachable_code;
}

static __cold __noreturn void CatchFireVarargs(char const * const file
                                             , size_t const line
                                             , char const * const cond
                                             , char const * const fmt, va_list args)
{
    if (DebugTerminal != nullptr && DebugTerminal->Capabilities->CanOutput)
    {
        DebugTerminal->WriteLine("");
        DebugTerminal->Write(">-- CAUGHT FIRE at line ");
        DebugTerminal->WriteUIntD(line);
        DebugTerminal->Write(" of \"");
        DebugTerminal->Write(file);
        DebugTerminal->WriteLine("\":");

        DebugTerminal->WriteLine(cond);

        if (fmt != nullptr)
            DebugTerminal->Write(fmt, args);
    }

    QuitProcess(HandleResult::ImmediateTermination, -1);

    //  Spin when things go haywire.
    while (true) ;
    __unreachable_code;
}

void Debug::CatchFireFormat(char const * const file
                          , size_t const line
                          , char const * const cond
                          , char const * const fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    CatchFireVarargs(file, line, cond, fmt, args);
    //  That function will never return.

    va_end(args);
}

void Debug::Assert(bool const condition
                 , char const * const file
                 , size_t const line
                 , char const * const msg)
{
    if unlikely(!condition)
        CatchFire(file, line, "", msg);
}

void Debug::AssertFormat(bool const condition
                       , char const * const file
                       , size_t const line
                       , char const * const fmt, ...)
{
    if unlikely(!condition)
    {
        va_list args;

        va_start(args, fmt);

        CatchFireVarargs(file, line, "", fmt, args);
        //  That function will never return either.

        va_end(args);
    }

    //  No reason to mess with the varargs otherwise.
}
