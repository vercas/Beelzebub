#pragma once

#include <keyboard.hpp>

#include <system/interrupts.hpp>
#include <system/cpu_instructions.hpp>

#ifdef __BEELZEBUB__DEBUG
#define breakpoint(...)                             \
do                                                  \
{                                                   \
    breakpointEscaped = 1;                          \
    if (Beelzebub::System::Interrupts::Enabled())   \
        do                                          \
        {                                           \
            CpuInstructions::Halt()                 \
        } while (breakpointEscaped);                \
    else                                            \
    {                                               \
        asm volatile ( "sti \n\t" : : : "memory" ); \
        do                                          \
        {                                           \
            CpuInstructions::Halt()                 \
        } while (breakpointEscaped);                \
        asm volatile ( "cli \n\t" : : : "memory" ); \
    }                                               \
} while (false);
#else
#define breakpoint(...) do {} while(false)
#endif

