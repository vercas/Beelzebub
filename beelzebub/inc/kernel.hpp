#pragma once

#define __BEELZEBUB__DEBUG  true

#include <architecture.h>
#include <metaprogramming.h>

namespace Beelzebub
{
    extern TerminalBase * MainTerminal;
    extern bool Scheduling;

    /**
     *  Entry point for the bootstrap processor.
     */
    __cold __bland void Main();

    /**
     *  Entry point for application processors.
     */
    __cold __bland void Secondary();
}
