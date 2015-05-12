#include <arc/system/cpu.hpp>
#include <kernel.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

volatile bool InitializingSystem = true;

TerminalBase * Beelzebub::MainTerminal;

void Beelzebub::Main()
{
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
    InitializingSystem = false;
    MainTerminal->WriteLine("Initialization complete!");

    //  Enable interrupts so they can run.
    MainTerminal->Write("[....] Enabling interrupts...");
    Cpu::EnableInterrupts();

    if (Cpu::InterruptsEnabled())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
        MainTerminal->WriteLine(" Fail..?\r[FAIL]");
    //  Can never bee too sure.

    MainTerminal->WriteLine("Halting indefinitely now.");

    //  Allow the CPU to rest.
    while (true) if (Cpu::CanHalt) Cpu::Halt();
}

void Beelzebub::Secondary()
{
    //  Wait for the system to initialize;
    while (InitializingSystem) ;

    //MainTerminal->Write('S');

    //  Enable interrupts so they can run.
    Cpu::EnableInterrupts();

    //  Allow the CPU to rest.
    while (true) if (Cpu::CanHalt) Cpu::Halt();
}
