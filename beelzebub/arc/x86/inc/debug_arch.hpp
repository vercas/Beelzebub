#pragma once

#include "stdarg.h"

#include <metaprogramming.h>
#include <handles.h>
#include <synchronization/spinlock.hpp>

#include <keyboard.h>

#include <kernel.hpp>

#include <system/cpu.hpp>

#ifdef __BEELZEBUB__DEBUG
#define breakpoint(...) \
do \
{ \
    breakpointEscaped = 1; \
    if (Beelzebub::System::Cpu::InterruptsEnabled()) \
        do \
        { \
        asm volatile ( "hlt \n\t" : : : "memory" ); \
        } while (breakpointEscaped); \
    else \
    { \
        asm volatile ( "sti \n\t" : : : "memory" ); \
        do \
        { \
            asm volatile ( "hlt \n\t" : : : "memory" ); \
        } while (breakpointEscaped); \
        asm volatile ( "cli \n\t" : : : "memory" ); \
    } \
} while (false);
#else
#define breakpoint(...) do {} while(false)
#endif

