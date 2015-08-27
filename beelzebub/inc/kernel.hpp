#pragma once

#include <metaprogramming.h>
#include <memory/manager.hpp>
#include <architecture.h>

namespace Beelzebub
{
    extern TerminalBase * MainTerminal;
    extern bool Scheduling;

    extern Memory::MemoryManager * BootstrapMemoryManager;

    /**
     *  Entry point for the bootstrap processor.
     */
    __cold __bland void Main();

    /**
     *  Entry point for application processors.
     */
    __cold __bland void Secondary();
}
