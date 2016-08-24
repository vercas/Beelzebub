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

#include <terminals/base.hpp>

#ifdef __BEELZEBUB_KERNEL
#include <synchronization/spinlock_uninterruptible.hpp>
#endif

//  NOTE: debug_arch.hpp is included near the end.

#define DEBUG_TERM if (Beelzebub::Debug::DebugTerminal != nullptr) \
    (*(Beelzebub::Debug::DebugTerminal)) 

#ifdef __BEELZEBUB_KERNEL
    #define DEBUG_TERM_ if (Beelzebub::Debug::DebugTerminal != nullptr) \
        withLock (Beelzebub::Debug::MsgSpinlock) \
            (*(Beelzebub::Debug::DebugTerminal)) 
#endif

#define ASSERT_1(cond) do {                                             \
if unlikely(!(cond))                                                    \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #cond         \
        , "ASSERTION FAILURE");                                         \
} while (false)

#define ASSERT_N(cond, ...) do {                                        \
if unlikely(!(cond))                                                    \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #cond         \
        , __VA_ARGS__);                                                 \
} while (false)

#define ASSERT(...) GET_MACRO100(__VA_ARGS__, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, \
ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_N, ASSERT_1)(__VA_ARGS__)

#define MSG(...) do {                                                   \
    if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
    {                                                                   \
        Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);      \
    }                                                                   \
} while (false)

#ifdef __BEELZEBUB_KERNEL
    #define MSG_(...) do {                                                  \
        if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
            withLock (Beelzebub::Debug::MsgSpinlock)                        \
                Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);  \
    } while (false)
#endif
//  Thse three are available for all configurations!

namespace Beelzebub { namespace Debug
{
    extern Terminals::TerminalBase * DebugTerminal;

#ifdef __BEELZEBUB_KERNEL
    extern Synchronization::SpinlockUninterruptible<> MsgSpinlock;
#else
    __shared __cold __bland Terminals::TerminalBase * GetDebugTerminal();
#endif

    __cold __noinline __noreturn void CatchFire(char const * const file
                                              , size_t const line
                                              , char const * const cond
                                              , char const * const msg);

    __cold __noinline __noreturn void CatchFireFormat(char const * const file
                                                    , size_t const line
                                                    , char const * const cond
                                                    , char const * const fmt, ...);

    __noinline void Assert(bool const condition
                         , char const * const file
                         , size_t const line
                         , char const * const msg);

    __noinline void AssertFormat(bool const condition
                               , char const * const file
                               , size_t const line
                               , char const * const fmt, ...);
}}

#ifdef __BEELZEBUB_KERNEL
    #include <debug_arch.hpp>

    #ifndef breakpoint
    #define breakpoint(...) do {} while (false)
    #endif
#endif

#define ASSERT_EQ_2(expected, val) do {                                         \
if unlikely((val) != (expected))                                                \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #val " == " #expected \
        , "Given value is not equal to the expected one.");                     \
} while (false)

#define ASSERT_EQ_3(fmt, expected, val) do {                                    \
if unlikely((val) != (expected))                                                \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #val " == " #expected \
        , "Expected " fmt ", got " fmt ".", expected, val);                     \
} while (false)

#define ASSERT_EQ_4(fmtE, fmtV, expected, val) do {                             \
if unlikely((val) != (expected))                                                \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #val " == " #expected \
        , "Expected " fmtE ", got " fmtV ".", expected, val);                   \
} while (false)

#define ASSERT_EQ(arg1, ...) GET_MACRO3(__VA_ARGS__, ASSERT_EQ_4, ASSERT_EQ_3, ASSERT_EQ_2)(arg1, __VA_ARGS__)

#define ASSERT_NEQ_2(expected, val) do {                                        \
if unlikely((val) == (expected))                                                \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #val " != " #expected \
        , "Expected anything but the given value.", expected);                  \
} while (false)

#define ASSERT_NEQ_3(fmt, expected, val) do {                                   \
if unlikely((val) == (expected))                                                \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #val " != " #expected \
        , "Expected anything but " fmt ".", expected);                          \
} while (false)

#define ASSERT_NEQ(arg1, ...) GET_MACRO2(__VA_ARGS__, ASSERT_NEQ_3, ASSERT_NEQ_2)(arg1, __VA_ARGS__)

//  And now the debug ones!

#ifdef __BEELZEBUB__DEBUG
    #define assert(...) ASSERT(__VA_ARGS__)

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

    #ifdef __BEELZEBUB_KERNEL
        #define msg_(...) do {                                                  \
            if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
                withLock (Beelzebub::Debug::MsgSpinlock)                        \
                    Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);  \
        } while (false)
    #endif
#else
    #define assert(...) while(false) \
        ASSERT(__VA_ARGS__)

    #define assert_or(cond, ...) \
        if (false) \
            ASSERT(cond, __VA_ARGS__); \
        else if unlikely(!(cond))

    #define msg(...) while (false) \
        Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__)

    #ifdef __BEELZEBUB_KERNEL
        #define msg_(...) msg(__VA_ARGS__)
    #endif
#endif
