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

#include <beel/exceptions.hpp>
#include <system/cpu.hpp>
#include <beel/terminals/base.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

ExceptionContext * * Beelzebub::GetExceptionContext()
{
    return &(Cpu::GetThread()->ExceptionContext);
}

void Beelzebub::LeaveExceptionContext()
{
    ExceptionContext * * const context = GetExceptionContext();

    if (*context != nullptr)
        *context = (*context)->Previous;
}

Exception * Beelzebub::GetException()
{
    return &(Cpu::GetThread()->Exception);
}

void Beelzebub::ThrowException()
{
    ExceptionContext * context = *(GetExceptionContext());

    if (context == nullptr)
        goto failure;

    while (context->Status != ExceptionContextStatus::Active)
        if (context->Previous != nullptr)
            context = context->Previous;
        else
            goto failure;
    //  This will find the first ready context.

    SwapToExceptionContext(context);
    __unreachable_code;
    //  Easy-peasy!

failure:
    UncaughtExceptionHandler();
    __unreachable_code;
}

void Beelzebub::UncaughtExceptionHandler()
{
    FAIL("Unhandled exception!");
}

/*  Now to implement some << operator magic.  */

namespace Beelzebub { namespace Terminals
{
    /*  First, the enums  */

    template<>
    TerminalBase & operator << <MemoryAccessType>(TerminalBase & term, MemoryAccessType const value)
    {
        term << (__underlying_type(MemoryAccessType))(value);

        if (0 != (value & MemoryAccessType::Unprivileged))
            term << " Unprivileged";
        if (0 != (value & MemoryAccessType::Unaligned))
            term << " Unaligned";

        switch (value & ~(MemoryAccessType::Unprivileged | MemoryAccessType::Unaligned))
        {
        case MemoryAccessType::Read:
            return term << " Read";
        case MemoryAccessType::Write:
            return term << " Write";
        case MemoryAccessType::Execute:
            return term << " Execute";
        default:
            return term << " UNKNOWN";
        }
    }

    template<>
    TerminalBase & operator << <MemoryLocationFlags>(TerminalBase & term, MemoryLocationFlags const value)
    {
        char specs[7] = " r  s)";
        specs[6] = '\0';

        if (0 != (value & MemoryLocationFlags::Present))        specs[0] = 'P';
        if (0 != (value & MemoryLocationFlags::Writable))       specs[1] = 'W';
        if (0 != (value & MemoryLocationFlags::Executable))     specs[2] = 'X';
        if (0 != (value & MemoryLocationFlags::Global))         specs[3] = 'G';
        if (0 != (value & MemoryLocationFlags::Userland))       specs[4] = 'U';

        return term << (__underlying_type(MemoryLocationFlags))(value) << " (" << specs;
    }

    template<>
    TerminalBase & operator << <MemoryAccessViolationData const *>(TerminalBase & term, MemoryAccessViolationData const * const value)
    {
        return term
            << "\tAddress: " << value->Address << EndLine
            << "\tFrame: " << value->PhysicalAddress << EndLine
            << "\tAccess type: " << value->AccessType << EndLine
            << "\tPage flags: " << value->PageFlags << EndLine;
    }

    template<>
    TerminalBase & operator << <UnitTestFailureData const *>(TerminalBase & term, UnitTestFailureData const * const value)
    {
        return term
            << "\tFile: " << value->FileName << EndLine
            << "\tLine: " << value->Line << EndLine;
    }

    template<>
    TerminalBase & operator << <Exception const *>(TerminalBase & term, Exception const * const value)
    {
        term << "Exception:" << EndLine
            << "\tType: " << value->Type << EndLine
            << "\tIP: " << reinterpret_cast<void *>(value->InstructionPointer) << EndLine
            << "\tSP: " << reinterpret_cast<void *>(value->StackPointer) << EndLine;

        switch (value->Type)
        {
        case ExceptionType::MemoryAccessViolation:
            return term << &(value->MemoryAccessViolation);
        case ExceptionType::UnitTestFailure:
            return term << &(value->UnitTestFailure);
        default:
            return term;
        }
    }
}}
