#include <system/cpu.hpp>
#include <synchronization/spinlock.hpp>
#include <kernel.hpp>
#include <debug.hpp>

#include <execution/thread_switching.hpp>
#include <execution/thread_init.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Execution;

volatile bool InitializingLock = true;
Spinlock InitializationLock;
Spinlock TerminalMessageLock;

TerminalBase * Beelzebub::MainTerminal;
bool Beelzebub::Scheduling;
MemoryManager * Beelzebub::BootstrapMemoryManager;
Thread BootstrapThread;

__bland static void ThreadTest(void* arg) {
    for (int i = 0; i < 5; ++i)
    {
        MainTerminal->Write("Hello from Thread! (Msg #");
        MainTerminal->WriteUIntD(i);
        MainTerminal->Write(") Running on Core #");
        MainTerminal->WriteUIntD(Cpu::GetIndex());
        MainTerminal->WriteLine();
    }

    DestroyThread(GetCurrentThread());
}

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

    Handle res = InitializeBootstrapThread(&BootstrapThread);

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
    {
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");

        assert(false, "Failed to initialize main entry point as bootstrap thread: %H"
            , res);
    }

    Thread Thrd;
    SpawnThread(&Thrd, ThreadTest);
    SetNext(&BootstrapThread, &Thrd);
    SwitchNext(&BootstrapThread);

    MainTerminal->WriteLine("\\Halting indefinitely now.");
    (&TerminalMessageLock)->Release();

    //  Allow the CPU to rest.
    while (true) if (Cpu::CanHalt) Cpu::Halt();
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
