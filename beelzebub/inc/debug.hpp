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

#include "stdarg.h"

#include <terminals/base.hpp>
#include <synchronization/spinlock_uninterruptible.hpp>

//  NOTE: debug_arch.hpp is included near the end.

#ifdef __BEELZEBUB__DEBUG
#define assert(cond, ...) do {                                          \
if unlikely(!(cond))                                                    \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #cond         \
        , __VA_ARGS__);                                                 \
} while (false)
#define assert_or(cond, ...) if unlikely(!(cond))                       \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #cond         \
        , __VA_ARGS__);                                                 \
else if (false)

//#define assert(cond, msg) Beelzebub::Debug::Assert(cond, __FILE__, __LINE__, msg)
#define msg(...) do {                                                   \
    if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
    {                                                                   \
        Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);      \
    }                                                                   \
} while (false)

#define msg_(...) do {                                                  \
    if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
        withLock (Beelzebub::Debug::MsgSpinlock)                        \
            Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);  \
} while (false)
#else
#define assert(...) do {} while(false)
#define assert_or(cond, ...) if unlikely(!(cond))

#define msg(...) do {} while(false)
#define msg_(...) do {} while(false)
#endif

#define ASSERT(cond, ...) do {                                          \
if unlikely(!(cond))                                                    \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #cond         \
        , __VA_ARGS__);                                                 \
} while (false)
#define MSG(...) do {                                                   \
    if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
    {                                                                   \
        Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);      \
    }                                                                   \
} while (false)
#define MSG_(...) do {                                                  \
    if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
        withLock (Beelzebub::Debug::MsgSpinlock)                        \
            Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);  \
} while (false)
//  Thse three are available for all configurations!

namespace Beelzebub { namespace Debug
{
    extern Terminals::TerminalBase * DebugTerminal;

    extern Synchronization::SpinlockUninterruptible<> MsgSpinlock;

    __cold __noinline __noreturn void CatchFire(char const * const file
                                              , size_t const line
                                              , char const * const cond
                                              , char const * const msg);

    __cold __noinline __noreturn void CatchFire(char const * const file
                                              , size_t const line
                                              , char const * const cond
                                              , char const * const fmt, va_list args);

    __cold __noinline __noreturn void CatchFireFormat(char const * const file
                                                    , size_t const line
                                                    , char const * const cond
                                                    , char const * const fmt, ...);

    __noinline void Assert(bool const condition
                         , char const * const file
                         , size_t const line
                         , char const * const msg);

    __noinline void Assert(bool const condition
                         , char const * const file
                         , size_t const line
                         , char const * const msg, va_list args);

    __noinline void AssertFormat(bool const condition
                               , char const * const file
                               , size_t const line
                               , char const * const fmt, ...);
}}

#include <debug_arch.hpp>

#ifndef breakpoint
#define breakpoint(...) do {} while (false)
#endif

#define ASSERT_EQ(fmt, expected, val) do {                                      \
if unlikely((val) != (expected))                                                \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #val " == " #expected \
        , "Expected " fmt ", got " fmt ".", expected, val);                     \
} while (false)

#define ASSERT_NEQ(fmt, expected, val) do {                                     \
if unlikely((val) == (expected))                                                \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #val " != " #expected \
        , "Expected anything but " fmt ".", expected);                          \
} while (false)
