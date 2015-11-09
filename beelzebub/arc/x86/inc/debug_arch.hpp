#pragma once

#include <keyboard.hpp>

#include <system/interrupts.hpp>
#include <system/cpu_instructions.hpp>

#define BREAKPOINT(...)                              \
do                                                   \
{                                                    \
    breakpointEscaped = 1;                           \
    if (Beelzebub::System::Interrupts::AreEnabled()) \
        do                                           \
        {                                            \
            CpuInstructions::Halt();                 \
        } while (breakpointEscaped);                 \
    else                                             \
    {                                                \
        asm volatile ( "sti \n\t" : : : "memory" );  \
        do                                           \
        {                                            \
            CpuInstructions::Halt();                 \
        } while (breakpointEscaped);                 \
        asm volatile ( "cli \n\t" : : : "memory" );  \
    }                                                \
} while (false)

#define BREAKPOINT_SET_AUX(val) \
do                              \
{                               \
    breakpointEscapedAux = val; \
} while(false);

#ifdef __BEELZEBUB__DEBUG
#define breakpoint BREAKPOINT
#else
#define breakpoint(...) do {} while(false)
#endif

