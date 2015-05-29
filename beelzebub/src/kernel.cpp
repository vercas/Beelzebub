#include <system/cpu.hpp>
#include <synchronization/spinlock.hpp>
#include <kernel.hpp>

#include <execution/thread_switching.hpp>
#include <execution/thread_init.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Execution;

volatile bool InitializingLock = true;
Spinlock InitializationLock;
Spinlock TerminalMessageLock;

TerminalBase * Beelzebub::MainTerminal;
bool Beelzebub::Scheduling;

////////////////////////////////////////////////////////

Thread tA, tB;

byte stackA[4096];
byte stackB[4096];

void DoA(void * const state)
{
    while (true)
    {
        for (size_t i = 0; i < 108; ++i)
            MainTerminal->Write('A');

        MainTerminal->Write("->");

        ScheduleNext(&tA);
    }
}

void DoB(void * const state)
{
    while (true)
    {
        for (size_t i = 0; i < 108; ++i)
            MainTerminal->Write('B');

        MainTerminal->Write("->");

        ScheduleNext(&tB);
    }
}

void StartThreadTest()
{
    tA.KernelStackBottom = (uintptr_t)stackA;
    tA.KernelStackTop = tA.KernelStackBottom + 4096;

    tB.KernelStackBottom = (uintptr_t)stackB;
    tB.KernelStackTop = tB.KernelStackBottom + 4096;

    tA.Next = tA.Previous = &tB;
    tB.Next = tB.Previous = &tA;

    tA.EntryPoint = &DoA;
    tB.EntryPoint = &DoB;

    InitializeThreadState(&tA);
    InitializeThreadState(&tB);

    uintptr_t dummy;
    SwitchThread(&dummy, tA.KernelStackPointer);
}

////////////////////////////////////////////////////////

void Beelzebub::Main()
{
    (&InitializationLock)->Acquire();
    InitializingLock = false;

    //  First step is getting a simple terminal running for the most
    //  basic of output. This is a platform-specific function.
    MainTerminal = InitializeTerminalProto();

    MainTerminal->WriteLine("Prototerminal initialized.");

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
    MainTerminal->WriteUIntD(Cpu::GetUnpreciseIndex());
    MainTerminal->WriteLine();

    //  Enable interrupts so they can run.
    MainTerminal->Write("|[....] Enabling interrupts...");
    Cpu::EnableInterrupts();

    if (Cpu::InterruptsEnabled())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");
    //  Can never bee too sure.

    MainTerminal->WriteLine("\\Attempting to run scheduler.");

    StartThreadTest();

    MainTerminal->WriteLine("\\Halting indefinitely now.");

    (&TerminalMessageLock)->Release();

    //  Allow the CPU to rest.
    while (true) if (Cpu::CanHalt) Cpu::Halt();
}

void Beelzebub::Secondary()
{
    //  Wait for the initialization spinlock to be ready.
    while (InitializingLock) ;

    //  Wait for the system to initialize.
    (&InitializationLock)->Spin();

    //  Now every core will print.
    (&TerminalMessageLock)->Acquire();
    MainTerminal->Write("+-- Core #");
    MainTerminal->WriteUIntD(Cpu::GetUnpreciseIndex());
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
