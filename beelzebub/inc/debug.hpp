#pragma once

#include "stdarg.h"

#include <metaprogramming.h>
#include <handles.h>
#include <synchronization/spinlock.hpp>

#include <kernel.hpp>

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

using namespace Beelzebub::Synchronization;

namespace Beelzebub { namespace Debug
{
    extern TerminalBase * DebugTerminal;

    extern Spinlock<> MsgSpinlock;

    __cold __bland __noinline __noreturn void CatchFire(const char * const file
                                                      , const size_t line
                                                      , const char * const msg);

    __cold __bland __noinline __noreturn void CatchFire(const char * const file
                                                      , const size_t line
                                                      , const char * const fmt, va_list args);

    __cold __bland __noinline __noreturn void CatchFireFormat(const char * const file
                                                            , const size_t line
                                                            , const char * const fmt, ...);

    __bland __noinline void Assert(const bool condition
                                 , const char * const file
                                 , const size_t line
                                 , const char * const msg);

    __bland __noinline void Assert(const bool condition
                                 , const char * const file
                                 , const size_t line
                                 , const char * const msg, va_list args);

    __bland __noinline void AssertFormat(const bool condition
                                       , const char * const file
                                       , const size_t line
                                       , const char * const fmt, ...);
}}

#include <debug_arch.hpp>

#ifndef breakpoint
#define breakpoint(...) do {} while (false)
#endif
