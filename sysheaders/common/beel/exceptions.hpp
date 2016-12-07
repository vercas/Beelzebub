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

#include <beel/structs.kernel.hpp>

#define withExceptionContext(name) with(Beelzebub::ExceptionGuard name)

#define __try \
    withExceptionContext(__x_ctxt_g) \
    if (Beelzebub::EnterExceptionContext(&(__x_ctxt_g.Context)))

#define __catch0() \
    else

#define __catch1(name) \
    else with (Beelzebub::Exception const * const name = Beelzebub::GetException())

#define __catch(...) GET_MACRO2(__dummy__, ##__VA_ARGS__, __catch1, __catch0)(__VA_ARGS__)

#define __x_suspend do { \
    if (__x_ctxt_g.Context.Status == ExceptionStatus::Active) \
        __x_ctxt_g.Context.Status = ExceptionStatus::Suspended; \
} while (false)

#define __x_resume do { \
    if (__x_ctxt_g.Context.Status == ExceptionStatus::Suspended) \
        __x_ctxt_g.Context.Status = ExceptionStatus::Active; \
} while (false)

namespace Beelzebub
{
    __extern ExceptionContext * * GetExceptionContext();
    __extern __returns_twice bool EnterExceptionContext(ExceptionContext * context);
    __extern void LeaveExceptionContext();
    __extern Exception * GetException();
    __extern __noreturn void ThrowException();

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
