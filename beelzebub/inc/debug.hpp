#pragma once

#include "stdarg.h"

#include <terminals/base.hpp>
#include <synchronization/spinlock.hpp>

//  NOTE: debug_arch.hpp is included near the end.

#ifdef __BEELZEBUB__DEBUG
#define assert(cond, ...) do {                                          \
if unlikely(!(cond))                                                    \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, __VA_ARGS__); \
} while (false)
#define assert_or(cond, ...) if unlikely(!(cond))                       \
    Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, __VA_ARGS__); \
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
    {                                                                   \
        (&Beelzebub::Debug::MsgSpinlock)->Acquire();                    \
        Beelzebub::Debug::DebugTerminal->WriteFormat(__VA_ARGS__);      \
        (&Beelzebub::Debug::MsgSpinlock)->Release();                    \
    }                                                                   \
} while (false)
#else
#define assert(...) do {} while(false)
#define assert_or(cond, ...) if unlikely(!(cond))

#define msg(...) do {} while(false)
#define msg_(...) do {} while(false)
#endif

namespace Beelzebub { namespace Debug
{
    extern Terminals::TerminalBase * DebugTerminal;

    extern Synchronization::Spinlock<> MsgSpinlock;

    __cold __bland __noinline __noreturn void CatchFire(char const * const file
                                                      , size_t const line
                                                      , char const * const msg);

    __cold __bland __noinline __noreturn void CatchFire(char const * const file
                                                      , size_t const line
                                                      , char const * const fmt, va_list args);

    __cold __bland __noinline __noreturn void CatchFireFormat(char const * const file
                                                            , size_t const line
                                                            , char const * const fmt, ...);

    __bland __noinline void Assert(bool const condition
                                 , char const * const file
                                 , size_t const line
                                 , char const * const msg);

    __bland __noinline void Assert(bool const condition
                                 , char const * const file
                                 , size_t const line
                                 , char const * const msg, va_list args);

    __bland __noinline void AssertFormat(bool const condition
                                       , char const * const file
                                       , size_t const line
                                       , char const * const fmt, ...);
}}

#include <debug_arch.hpp>

#ifndef breakpoint
#define breakpoint(...) do {} while (false)
#endif
