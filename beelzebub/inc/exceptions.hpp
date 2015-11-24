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

#pragma once

#include <metaprogramming.h>
//  <exceptions.arc.hpp> is included at the end!

#define withExceptionContext(name) with(Beelzebub::ExceptionGuard name)

#define __try \
    withExceptionContext(MCATS(_excp_guard_, __LINE__)) \
    if (Beelzebub::EnterExceptionContext(&(MCATS(_excp_guard_, __LINE__).Context)))

#define __catch(name) \
    else with (Beelzebub::Exception  const* const name = Beelzebub::GetException())

namespace Beelzebub
{
    enum class ExceptionType : uintptr_t
    {
        //  This one indicates that there is no exception.
        None                    = 0,
        //  Null pointer dereference.
        NullReference           = 1,
        //  Illegal memory access.
        MemoryAccessViolation   = 2,
        //  Integral division by zero.
        DivideByZero            = 3,
        //  Checked arithmeetic overflow.
        ArithmeticOverflow      = 4,
        //  Invalid instruction encoding or opcode.
        InvalidInstruction      = 5,

        //  Haywire.
        Unknown                 = ~((uintptr_t)0),
    };

    enum class MemoryAccessType : uint8_t
    {
        Read        = 0,
        Write       = 1,
        Execute     = 2,

        Unaligned   = 1 << 7,
    };

    ENUMOPS(MemoryAccessType)

    enum class MemoryFlags : uint16_t
    {
        None        = 0,

        //  The memory at the given (virtual) address is mapped.
        Present     = 1 << 0,
        //  The memory may be written.
        Writable    = 1 << 1,
        //  The memory may be executed.
        Executable  = 1 << 2,
        //  The memory is global (shared by all processes).
        Global      = 1 << 3,
        //  The memory is accessible by userland.
        Userland    = 1 << 4,
        //  Previous access to this area of memory have occured.
        Accessed    = 1 << 5,
        //  This area of memory has been written to.
        Written     = 1 << 6,
    };

    ENUMOPS(MemoryFlags)

    struct MemoryAccessViolationData
    {
        //  These are ordered by size, descending.

        paddr_t PhysicalAddress;
        void * Address;
        //  Physical address may be larger (PAE).

        MemoryFlags PageFlags;
        MemoryAccessType AccessType;
    };

    struct Exception
    {
        ExceptionType Type;
        uintptr_t InstructionPointer;
        uintptr_t StackPointer;

        union
        {
            MemoryAccessViolationData MemoryAccessViolation;
        };
    };
}

#include <exceptions.arc.hpp>

namespace Beelzebub
{
    __extern __hot __used ExceptionContext * * GetExceptionContext();

    __extern __hot __returns_twice bool EnterExceptionContext(ExceptionContext * context);
    //  Defined in assembly.

    __hot void LeaveExceptionContext();
    //  Defined in code file to avoid dependency hell.

    __hot Exception * GetException();

    /// <summary>Guards a scope with an exception context.</summary>
    struct ExceptionGuard
    {
        /*  Constructor(s)  */

        inline ExceptionGuard() : Context() { }
        //  I hope this doesn't explicitly initialize the context.
        //  Also, this doesn't enter the context.

        ExceptionGuard(ExceptionGuard const &) = delete;
        ExceptionGuard(ExceptionGuard && other) = delete;
        ExceptionGuard & operator =(ExceptionGuard const &) = delete;
        ExceptionGuard & operator =(ExceptionGuard &&) = delete;

        /*  Destructor  */

        __forceinline ~ExceptionGuard()
        {
            LeaveExceptionContext();
        }

        /*  Field(s)  */

        ExceptionContext Context;
    };
}
