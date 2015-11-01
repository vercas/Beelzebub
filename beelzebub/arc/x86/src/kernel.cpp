#include <kernel.hpp>
#include <entry.h>

#include <terminals/serial.hpp>
#include <terminals/vbe.hpp>
#include <multiboot.h>

#include <system/cpu.hpp>
#include <execution/thread_init.hpp>

#include <memory/manager_amd64.hpp>
#include <system/acpi.hpp>

#include <synchronization/spinlock.hpp>
#include <debug.hpp>

#if __BEELZEBUB__TEST_STR
#include <tests/string.hpp>
#endif

#if __BEELZEBUB__TEST_OBJA
#include <tests/object_allocator.hpp>
#endif

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

volatile bool InitializingLock = true;
Spinlock<> InitializationLock;
Spinlock<> TerminalMessageLock;

/*  System Globals  */

TerminalBase * Beelzebub::MainTerminal;
bool Beelzebub::Scheduling;

Process Beelzebub::BootstrapProcess;
Thread Beelzebub::BootstrapThread;

Domain Beelzebub::Domain0;

/*******************
    ENTRY POINTS
*******************/

/*  Main entry point  */

void Beelzebub::Main()
{
    InitializingLock = true;    //  Makin' sure.
    (&InitializationLock)->Acquire();
    InitializingLock = false;

    Handle res;
    //  Used for intermediary results.

    new (&Domain0) Domain();
    //  Initialize domain 0. Make sure it's not in a possibly-invalid state.

    //  First step is getting a simple terminal running for the most
    //  basic of output. This should be x86-common.
    MainTerminal = InitializeTerminalProto();

    MainTerminal->WriteLine("Prototerminal initialized!");

    //  Setting up basic interrupt handlers 'n stuff.
    //  Again, platform-specific.
    MainTerminal->Write("[....] Initializing interrupts...");
    res = InitializeInterrupts();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
    {
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");

        assert(false, "Failed to initialize interrupts: %H"
            , res);
    }

    //  Initialize the memory by partition and allocation.
    //  Differs on IA-32 and AMD64. May tweak virtual memory in the process.
    MainTerminal->Write("[....] Initializing physical memory...");
    res = InitializePhysicalMemory();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
    {
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");

        assert(false, "Failed to initialize physical memory: %H"
            , res);
    }

    //  Initialize the virtual memory for use by the kernel.
    //  Differs on IA-32 and AMD64.
    MainTerminal->Write("[....] Initializing virtual memory...");
    res = InitializeVirtualMemory();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
    {
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");

        assert(false, "Failed to initialize virtual memory: %H"
            , res);
    }

    //  Initialize the memory for partition and allocation.
    //  Mostly common on x86.
    MainTerminal->Write("[....] Initializing other processing units...");
    res = InitializeProcessingUnits();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
    {
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");

        assert(false, "Failed to initialize the other processing units: %H"
            , res);
    }

    //  Initialize the modules loaded with the kernel.
    //  Mostly common.
    MainTerminal->Write("[....] Initializing modules...");
    res = InitializeModules();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
    {
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");

        assert(false, "Failed to initialize modules: %H"
            , res);
    }

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
    TerminalMessageLock.Acquire();
    MainTerminal->Write("+-- Core #");
    MainTerminal->WriteUIntD(Cpu::GetIndex());
    MainTerminal->WriteLine();

    //  Enable interrupts so they can run.
    MainTerminal->Write("|[....] Enabling interrupts...");
    Interrupts::Enable();

    if (Interrupts::AreEnabled())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
        //  Can never bee too sure.
    else
    {
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");

        assert(false, "Enabling interrupts failed!");
    }

    MainTerminal->Write("|[....] Initializing as bootstrap thread...");

    res = InitializeBootstrapThread(&BootstrapThread, &BootstrapProcess, &BootstrapMemoryManager);

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

#ifdef __BEELZEBUB__TEST_OBJA
    ObjectAllocatorTestBarrier1.Reset();
    ObjectAllocatorTestBarrier2.Reset();
    ObjectAllocatorTestBarrier3.Reset();
#endif

    TerminalMessageLock.Release();

#ifdef __BEELZEBUB__TEST_OBJA
    TerminalMessageLock.Acquire();
    MainTerminal->WriteFormat("Core %us: Testing fixed-sized object allocator.%n", Cpu::GetIndex());
    TerminalMessageLock.Release();

    TestObjectAllocator(true);

    TerminalMessageLock.Acquire();
    MainTerminal->WriteFormat("Core %us: Finished object allocator test.%n", Cpu::GetIndex());
    TerminalMessageLock.Release();
#endif

    //  Allow the CPU to rest.
    while (true)
    {
        if (CpuInstructions::CanHalt) CpuInstructions::Halt();

        TerminalMessageLock.Acquire();
        MainTerminal->WriteLine(">>-- Rehalting! --<<");
        TerminalMessageLock.Release();
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
    TerminalMessageLock.Acquire();
    MainTerminal->Write("+-- Core #");
    MainTerminal->WriteUIntD(Cpu::GetIndex());
    MainTerminal->WriteLine();

    //  Enable interrupts so they can run.
    MainTerminal->Write("|[....] Enabling interrupts...");
    Interrupts::Enable();

    if (Interrupts::AreEnabled())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
        MainTerminal->WriteLine(" Fail..?\r|[FAIL]");
    //  Can never bee too sure.

    MainTerminal->WriteLine("\\Halting indefinitely now.");

    TerminalMessageLock.Release();

#ifdef __BEELZEBUB__TEST_OBJA
    TerminalMessageLock.Acquire();
    MainTerminal->WriteFormat("Core %us: Testing fixed-sized object allocator.%n", Cpu::GetIndex());
    TerminalMessageLock.Release();

    TestObjectAllocator(false);

    TerminalMessageLock.Acquire();
    MainTerminal->WriteFormat("Core %us: Finished object allocator test.%n", Cpu::GetIndex());
    TerminalMessageLock.Release();
#endif

    //  Allow the CPU to rest.
    while (true) if (CpuInstructions::CanHalt) CpuInstructions::Halt();
}

/****************
    TERMINALS
****************/

SerialTerminal initialSerialTerminal;
VbeTerminal initialVbeTerminal;

TerminalBase * InitializeTerminalProto()
{
    //  TODO: Properly retrieve these addresses.

    //  Initializes COM1.
    //COM1 = ManagedSerialPort(0x3F8);
    new (&COM1) ManagedSerialPort(0x3F8);
    COM1.Initialize();

    //  Initializes the serial terminal.
    new (&initialSerialTerminal) SerialTerminal(&COM1);

    Beelzebub::Debug::DebugTerminal = &initialSerialTerminal;

    auto mbi = (multiboot_info_t *)JG_INFO_ROOT_EX->multiboot_paddr;

    new (&initialVbeTerminal) VbeTerminal((uintptr_t)mbi->framebuffer_addr, (uint16_t)mbi->framebuffer_width, (uint16_t)mbi->framebuffer_height, (uint16_t)mbi->framebuffer_pitch, (uint8_t)(mbi->framebuffer_bpp / 8));

    //Beelzebub::Debug::DebugTerminal = &initialVbeTerminal;

    msg("VM: %Xp; W: %u2, H: %u2, P: %u2; BPP: %u1.%n"
        , (uintptr_t)mbi->framebuffer_addr
        , (uint16_t)mbi->framebuffer_width, (uint16_t)mbi->framebuffer_height
        , (uint16_t)mbi->framebuffer_pitch, (uint8_t)mbi->framebuffer_bpp);

    /*msg(" vbe_control_info: %X4%n", mbi->vbe_control_info);
    msg(" vbe_mode_info: %X4%n", mbi->vbe_mode_info);
    msg(" vbe_mode: %X4%n", mbi->vbe_mode);
    msg(" vbe_interface_seg: %X4%n", mbi->vbe_interface_seg);
    msg(" vbe_interface_off: %X2%n", mbi->vbe_interface_off);
    msg(" vbe_interface_len: %X2%n", mbi->vbe_interface_len); //*/

    //  And returns it.
    //return &initialSerialTerminal; // termPtr;
    return &initialVbeTerminal;
}

TerminalBase * InitializeTerminalMain()
{
    return &initialVbeTerminal;
}

/***********************
    PROCESSING UNITS
***********************/

/**
 *  <summary>
 *  Initializes the other processing units in the system.
 *  </summary>
 */
Handle InitializeProcessingUnits()
{
    RsdpTable rsdp = Acpi::Initialize(MemoryManagerAmd64::IsaDmaStart + 0x0E0000
                                    , MemoryManagerAmd64::IsaDmaStart + 0x100000);

    if (rsdp.GetVersion() == AcpiVersion::v1)
        msg("[[ RSDP @ %Xp, v1 ]]%n", rsdp.GetVersion1());
    else
        msg("[[ RSDP @ %Xp, v2 ]]%n", rsdp.GetVersion2());

    return HandleResult::Okay;
}
