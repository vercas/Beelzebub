#include <system/cpu.hpp>
#include <synchronization/spinlock.hpp>
#include <kernel.hpp>
#include <debug.hpp>

#include <execution/thread_switching.hpp>
#include <execution/thread_init.hpp>

#if __BEELZEBUB__TEST_STR
#include <tests/string.hpp>
#endif

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Execution;

volatile bool InitializingLock = true;
Spinlock<> InitializationLock;
Spinlock<> TerminalMessageLock;

TerminalBase * Beelzebub::MainTerminal;
bool Beelzebub::Scheduling;

MemoryManager * Beelzebub::BootstrapMemoryManager;
Process Beelzebub::BootstrapProcess;
Thread Beelzebub::BootstrapThread;

/*  Main entry point  */

void Beelzebub::Main()
{
    (&InitializationLock)->Acquire();
    InitializingLock = false;

    //  First step is getting a simple terminal running for the most
    //  basic of output. This is a platform-specific function.
    MainTerminal = InitializeTerminalProto();

    MainTerminal->WriteLine("Prototerminal initialized.");

    //breakpoint();

    //  Initialize the memory for partition and allocation.
    //  Also platform-specific.
    MainTerminal->Write("[....] Initializing memory...");
    InitializeMemory();
    MainTerminal->WriteLine(" Done.\r[OKAY]");

    //  Initialize the modules loaded with the kernel.
    //  Also platform-specific.
    MainTerminal->Write("[....] Initializing modules...");
    InitializeModules();
    MainTerminal->WriteLine(" Done.\r[OKAY]");

    //  Setting up basic interrupt handlers 'n stuff.
    //  Again, platform-specific.
    MainTerminal->Write("[....] Initializing interrupts...");
    InitializeInterrupts();
    MainTerminal->WriteLine(" Done.\r[OKAY]");

    //  Upgrade the terminal to a more capable and useful one.
    //  Yet again, platform-specific.
    MainTerminal->Write("[....] Initializing main terminal...");
    TerminalBase * secondaryTerminal = InitializeTerminalMain();
    MainTerminal->WriteLine(" Done.\r[OKAY]");

    MainTerminal->WriteLine("Switching over.");
    MainTerminal = secondaryTerminal;

    //  Permit other processors to initialize themselves.
    MainTerminal->WriteLine("Initialization complete!");
    MainTerminal->WriteLine();
    (&InitializationLock)->Release();

    //  Now every core will print.
    (&TerminalMessageLock)->Acquire();
    MainTerminal->Write("+-- Core #");
    MainTerminal->WriteUIntD(Cpu::GetIndex());
    MainTerminal->WriteLine();

    //  Enable interrupts so they can run.
    MainTerminal->Write("|[....] Enabling interrupts...");
    Cpu::EnableInterrupts();

    if (Cpu::InterruptsEnabled())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
        //  Can never bee too sure.
    else
    {
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");

        assert(false, "Enabling interrupts failed!");
    }

    MainTerminal->Write("|[....] Initializing as bootstrap thread...");

    Handle res = InitializeBootstrapThread(&BootstrapThread, &BootstrapProcess, BootstrapMemoryManager);

    Cpu::SetActiveThread(&BootstrapThread);

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
    {
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");

        assert(false, "Failed to initialize main entry point as bootstrap thread: %H"
            , res);
    }

#ifdef __BEELZEBUB__TEST_STR
    MainTerminal->Write(">Testing string.h implementation...");

    TestStringLibrary();

    MainTerminal->WriteLine(" Done.");
#endif

#ifdef __BEELZEBUB__TEST_MT
    MainTerminal->Write(">Starting multitasking test...");

    StartMultitaskingTest();

    MainTerminal->WriteLine(" Done.");
#endif

    MainTerminal->WriteLine("\\Halting indefinitely now.");

    (&TerminalMessageLock)->Release();

    //  Allow the CPU to rest.
    while (true)
    {
        if (Cpu::CanHalt) Cpu::Halt();

        (&TerminalMessageLock)->Acquire();
        MainTerminal->WriteLine(">>-- Rehalting! --<<");
        (&TerminalMessageLock)->Release();
    }
}

/*  Secondary entry point  */

void Beelzebub::Secondary()
{
    //  Wait for the initialization spinlock to be ready.
    while (InitializingLock) ;

    //  Wait for the system to initialize.
    (&InitializationLock)->Spin();

    //  Now every core will print.
    (&TerminalMessageLock)->Acquire();
    MainTerminal->Write("+-- Core #");
    MainTerminal->WriteUIntD(Cpu::GetIndex());
    MainTerminal->WriteLine();

    //  Enable interrupts so they can run.
    MainTerminal->Write("|[....] Enabling interrupts...");
    Cpu::EnableInterrupts();

    if (Cpu::InterruptsEnabled())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");
    //  Can never bee too sure.

    MainTerminal->WriteLine("\\Halting indefinitely now.");

    (&TerminalMessageLock)->Release();

    //  Allow the CPU to rest.
    while (true) if (Cpu::CanHalt) Cpu::Halt();
}
