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

#include <beel/terminals/base.hpp>
#include <beel/debug.funcs.h>

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

#define FAIL_0()  __extension__ ({                                      \
    Beelzebub::Debug::CatchFire(__FILE__, __LINE__, nullptr             \
        , nullptr);                                                     \
    __unreachable_code;                                                 \
})

#define FAIL_N(...)  __extension__ ({                                   \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, nullptr       \
        , __VA_ARGS__);                                                 \
    __unreachable_code;                                                 \
})

#define FAIL(...) GET_MACRO100(DUMMEH, ##__VA_ARGS__, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_N, \
FAIL_N, FAIL_N, FAIL_N, FAIL_N, FAIL_0)(__VA_ARGS__)

#define MSG(...)  __extension__ ({                                      \
    if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
        Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);      \
})

#ifdef __BEELZEBUB_KERNEL
    #define MSG_(...)  __extension__ ({                                     \
        if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
            withLock (Beelzebub::Debug::MsgSpinlock)                        \
                Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);  \
    })
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

    __noinline void Assert(bool const condition
                         , char const * const file
                         , size_t const line
                         , char const * const msg);

    __noinline void Assert(const bool condition
                         , const char * const file
                         , const size_t line
                         , const char * const msg
                         , va_list args);

    __noinline void AssertFormat(bool const condition
                               , char const * const file
                               , size_t const line
                               , char const * const fmt
                               , ...);

    /* Inspired by http://www.drdobbs.com/cpp/enhancing-assertions/184403745 */
    class AssertHelper
    {
    public:
        /*  Constructor(s)  */

        inline AssertHelper(Terminals::TerminalBase * term)
            : Term(*term)
            , AssertHelperAlpha(*this)
            , AssertHelperBeta(*this)
            , State(0)
            , WroteParameters(false)
        {

        }

        /*  State  */

        bool RealityCheck();

        /*  Prints  */

        AssertHelper & DumpContext(char const * file, size_t line
                                 , char const * cond
                                 , char const * fmt, ...);

        template<typename T>
        inline AssertHelper & DumpParameter(char const * name, T const val)
        {
            if unlikely(!this->WroteParameters)
            {
                this->Term << "Parameters:" << Terminals::EndLine;

                this->WroteParameters = true;
            }

            this->Term << '\t' << name << ": " << val << Terminals::EndLine;

            return *this;
        }

        /*  Fields  */

        Terminals::TerminalBase & Term;
        AssertHelper & AssertHelperAlpha, & AssertHelperBeta;

        int State;
        bool WroteParameters;
    };
}}

#ifdef __BEELZEBUB_KERNEL
    #include <debug_arch.hpp>

    #ifndef breakpoint
    #define breakpoint(...) do {} while (false)
    #endif
#endif

#define AssertHelperAlpha_1(x) AssertHelperAlpha_2(#x, x)
#define AssertHelperBeta_1(x) AssertHelperBeta_2(#x, x)

#define AssertHelperAlpha_2(name, x) AssertHelperOmega(Beta, name, x)
#define AssertHelperBeta_2(name, x) AssertHelperOmega(Alpha, name, x)

#define AssertHelperAlpha(...) GET_MACRO2(__VA_ARGS__, AssertHelperAlpha_2, AssertHelperAlpha_1)(__VA_ARGS__)
#define AssertHelperBeta(...)  GET_MACRO2(__VA_ARGS__, AssertHelperBeta_2 , AssertHelperBeta_1 )(__VA_ARGS__)

#define AssertHelperOmega(next, name, x) \
    AssertHelperAlpha.DumpParameter(name, (x)).AssertHelper##next

#define ASSERT_1(cond) \
    if unlikely(!(cond)) \
        with (Beelzebub::Debug::AssertHelper MCATS(_assh_, __LINE__) { Beelzebub::Debug::DebugTerminal }) \
            while (MCATS(_assh_, __LINE__).RealityCheck() || true) \
                MCATS(_assh_, __LINE__).DumpContext(__FILE__, __LINE__, #cond, nullptr).AssertHelperAlpha

#define ASSERT_N(cond, ...) \
    if unlikely(!(cond)) \
        with (Beelzebub::Debug::AssertHelper MCATS(_assh_, __LINE__) { Beelzebub::Debug::DebugTerminal }) \
            while (MCATS(_assh_, __LINE__).RealityCheck() || true) \
                MCATS(_assh_, __LINE__).DumpContext(__FILE__, __LINE__, #cond, __VA_ARGS__).AssertHelperAlpha

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

#define XEND ; __unreachable_code; })()

#define ASSERTX_COMMON(cond) \
    if unlikely(!(cond)) ([=]() __attribute__((__noinline__, __cold__, __noreturn__, __optimize__("Os"), __section__(".text.assertions"))) -> void { \
        Beelzebub::Debug::AssertHelper MCATS(_assh_, __LINE__) { Beelzebub::Debug::DebugTerminal }; \
        while (MCATS(_assh_, __LINE__).RealityCheck() || true) \
            MCATS(_assh_, __LINE__)

#define ASSERTX_1(cond) \
    ASSERTX_COMMON(cond).DumpContext(__FILE__, __LINE__, #cond, nullptr).AssertHelperAlpha

#define ASSERTX_N(cond, ...) \
    ASSERTX_COMMON(cond).DumpContext(__FILE__, __LINE__, #cond, __VA_ARGS__).AssertHelperAlpha

#define ASSERTX(...) GET_MACRO100(__VA_ARGS__, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, \
ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_N, ASSERTX_1)(__VA_ARGS__)
/*
#define ASSERT_EQ_2(expec, given) \
    if unlikely((expec) != (given)) \
        with (Beelzebub::Debug::AssertHelper MCATS(_assh_, __LINE__) { Beelzebub::Debug::DebugTerminal }) \
            while (MCATS(_assh_, __LINE__).RealityCheck() || true) \
                MCATS(_assh_, __LINE__).DumpContext(__FILE__, __LINE__, #expec " == " #given, nullptr) \
                .DumpParameter("expected", (expec)).DumpParameter("given", (given)).AssertHelperAlpha
*/
#define ASSERT_EQ_2(expec, given) \
    ASSERTX_COMMON((expec) == (given)).DumpContext(__FILE__, __LINE__, #expec " == " #given, nullptr) \
    .DumpParameter("expected", (expec)).DumpParameter("given", (given)).AssertHelperAlpha XEND

#define ASSERT_EQ_3(fmt, expec, given) \
    ASSERTX((expec) == (given), "Expected " fmt ", given " fmt ".", (expec), (given))XEND

#define ASSERT_EQ_4(fmtE, fmtG, expec, given) \
    ASSERTX((expec) == (given), "Expected " fmtE ", given " fmtG ".", (expec), (given))XEND

#define ASSERT_EQ(arg1, ...) GET_MACRO3(__VA_ARGS__, ASSERT_EQ_4, ASSERT_EQ_3, ASSERT_EQ_2)(arg1, __VA_ARGS__)
/*
#define ASSERT_NEQ_2(expec, given) \
    if unlikely((expec) == (given)) \
        with (Beelzebub::Debug::AssertHelper MCATS(_assh_, __LINE__) { Beelzebub::Debug::DebugTerminal }) \
            while (MCATS(_assh_, __LINE__).RealityCheck() || true) \
                MCATS(_assh_, __LINE__).DumpContext(__FILE__, __LINE__, #expec " != " #given, nullptr) \
                .DumpParameter("expected", (expec)).DumpParameter("given", (given)).AssertHelperAlpha
*/
#define ASSERT_NEQ_2(expec, given) \
    ASSERTX_COMMON((expec) != (given)).DumpContext(__FILE__, __LINE__, #expec " != " #given, nullptr) \
    .DumpParameter("did not expect", (expec)).AssertHelperAlpha XEND

#define ASSERT_NEQ_3(fmt, expec, given) \
    ASSERTX((expec) != (given), "Expected anything but " fmt ".", (expec))XEND

#define ASSERT_NEQ(arg1, ...) GET_MACRO2(__VA_ARGS__, ASSERT_NEQ_3, ASSERT_NEQ_2)(arg1, __VA_ARGS__)

//  And now the debug ones!

#ifdef __BEELZEBUB__DEBUG
    #define fail(...) FAIL(__VA_ARGS__)

    #define assert(...) ASSERT(__VA_ARGS__)

    #define assert_or(cond, ...) if unlikely(!(cond))                       \
        Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, #cond         \
            , __VA_ARGS__);                                                 \
    else if (false)

    //#define assert(cond, msg) Beelzebub::Debug::Assert(cond, __FILE__, __LINE__, msg)
    #define msg(...)  __extension__ ({                                      \
        if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
            Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);      \
    })

    #ifdef __BEELZEBUB_KERNEL
        #define msg_(...)  __extension__ ({                                     \
            if likely(Beelzebub::Debug::DebugTerminal != nullptr)               \
                withLock (Beelzebub::Debug::MsgSpinlock)                        \
                    Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);  \
        })
    #endif
#else
    #define fail(...) __unreachable_code

    // #define assert(cond, ...) do { if (!(cond)) { __unreachable_code; } } while (false)
    // //  `unlikely` is not use here to make the compiler understand this more easily.
    // //  Tests show that it gets the clue.

    #define assert(cond, ...) \
        if (!(cond)) __unreachable_code; \
        if (false) \
            Beelzebub::Debug::AssertHelper(Beelzebub::Debug::DebugTerminal).AssertHelperAlpha

    #define assert_or(cond, ...) \
        if unlikely(!(cond))

    #define msg(...)  __extension__ ({                                 \
        if (false)                                                     \
            Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__); \
    })

    #ifdef __BEELZEBUB_KERNEL
        #define msg_(...) msg(__VA_ARGS__)
    #endif
#endif
