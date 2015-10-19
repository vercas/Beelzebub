#pragma once

#include <metaprogramming.h>
#include <memory/manager.hpp>
#include <memory/page_allocator.hpp>
#include <execution/thread_init.hpp>
#include <architecture.h>

namespace Beelzebub
{
    extern TerminalBase * MainTerminal;
    extern bool Scheduling;

    extern Memory::PageAllocator * MainPageAllocator;
    extern Memory::MemoryManager * BootstrapMemoryManager;

    extern Execution::Process BootstrapProcess;
    extern Execution::Thread BootstrapThread;

    /**
     *  Entry point for the bootstrap processor.
     */
    __cold __bland void Main();

    /**
     *  Entry point for application processors.
     */
    __cold __bland void Secondary();
}
