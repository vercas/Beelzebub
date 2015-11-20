#pragma once

#include <system/domain.hpp>    //  Platform-specific.
#include <execution/thread.hpp>
#include <terminals/base.hpp>

namespace Beelzebub
{
    extern TerminalBase * MainTerminal;
    extern bool Scheduling;
    extern bool CpuDataSetUp;

    extern Execution::Process BootstrapProcess;
    extern Execution::Thread BootstrapThread;

    extern System::Domain Domain0;

    /**
     *  <summary>Entry point for the bootstrap processor.</summary>
     */
    __cold __bland void Main();

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    /**
     *  <summary>Entry point for application processors.</summary>
     */
    __cold __bland void Secondary();

    /**
     *  <summary>Entry point for other domains.</summary>
     */
    __cold __bland void Ternary();
#endif
}
