#include <kernel.hpp>
#include <entry.h>

#include <terminals/serial.hpp>
#include <terminals/vbe.hpp>
#include <multiboot.h>

#include <system/cpu.hpp>
#include <execution/thread_init.hpp>

#include <system/interrupt_controllers/pic.hpp>
#include <system/interrupt_controllers/lapic.hpp>
#include <system/interrupt_controllers/ioapic.hpp>
#include <system/timers/pit.hpp>

#include <memory/manager_amd64.hpp>
#include <system/acpi.hpp>

#include <ap_bootstrap.hpp>

#include <synchronization/spinlock.hpp>
#include <utils/wait.hpp>
#include <string.h>

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
using namespace Beelzebub::System::Timers;
using namespace Beelzebub::System::InterruptControllers;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::Utils;

/*  Synchronization  */

//SmpBarrier InitializationBarrier {};

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
    InitializationLock.Acquire();

    Handle res;
    //  Used for intermediary results.

    new (&Domain0) Domain();
    //  Initialize domain 0. Make sure it's not in a possibly-invalid state.

    //  First step is getting a simple terminal running for the most
    //  basic of output. This should be x86-common.
    MainTerminal = InitializeTerminalProto();

    MainTerminal->WriteLine("Welcome to Beelzebub!                            (c) 2015 Alexandru-Mihai Maftei");

    //  Setting up basic interrupt handlers 'n stuff.
    //  Again, platform-specific.
    MainTerminal->Write("[....] Initializing interrupts...");
    res = InitializeInterrupts();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize interrupts: %H"
            , res);
    }

    //  Preparing the PIT for basic timing.
    //  Common on x86.
    MainTerminal->Write("[....] Initializing PIT...");
    res = InitializePit();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize the PIT: %H"
            , res);
    }

    //  Initialize the memory by partition and allocation.
    //  Differs on IA-32 and AMD64. May tweak virtual memory in the process.
    MainTerminal->Write("[....] Initializing physical memory...");
    res = InitializePhysicalMemory();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize physical memory: %H"
            , res);
    }

    //  Initialize the virtual memory for use by the kernel.
    //  Differs on IA-32 and AMD64.
    MainTerminal->Write("[....] Initializing virtual memory...");
    res = InitializeVirtualMemory();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize virtual memory: %H"
            , res);
    }

    //  Initialize the ACPI tables for easier use.
    //  Mostly common on x86.
    MainTerminal->Write("[....] Initializing ACPI tables...");
    res = InitializeAcpiTables();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize the ACPI tables: %H"
            , res);
    }

    //  Initialize the LAPIC for the BSP and the I/O APIC.
    //  Mostly common on x86.
    MainTerminal->Write("[....] Initializing APIC...");
    res = InitializeApic();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize the APIC: %H"
            , res);
    }

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    MainTerminal->WriteLine("[SKIP] Kernel was build with SMP disabled. Other processing units ignored.");
#else
    //  Initialize the other processing units in the system.
    //  Mostly common on x86, but the executed code differs by arch.
    if (Acpi::PresentLapicCount > 1)
    {
        MainTerminal->Write("[....] Initializing extra processing units...");
        res = InitializeProcessingUnits();

        if (res.IsOkayResult())
            MainTerminal->WriteLine(" Done.\r[OKAY]");
        else
        {
            MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

            ASSERT(false, "Failed to initialize the extra processing units: %H"
                , res);
        }
    }
    else
    {
        MainTerminal->WriteLine("[SKIP] No extra processing units available.");
    }
#endif

    //  Initialize the modules loaded with the kernel.
    //  Mostly common.
    MainTerminal->Write("[....] Initializing modules...");
    res = InitializeModules();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize modules: %H"
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
    InitializationLock.Release();

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

        ASSERT(false, "Enabling interrupts failed!");
    }

    MainTerminal->Write("|[....] Initializing as bootstrap thread...");

    res = InitializeBootstrapThread(&BootstrapThread, &BootstrapProcess, &BootstrapMemoryManager);

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r|[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r|[FAIL]%n", res);

        ASSERT(false, "Failed to initialize main entry point as bootstrap thread: %H"
            , res);
    }

    Cpu::SetActiveThread(&BootstrapThread);

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

        //TerminalMessageLock.Acquire();
        //MainTerminal->WriteLine(">>-- Rehalting! --<<");
        //TerminalMessageLock.Release();
    }
}

#if   defined(__BEELZEBUB_SETTINGS_SMP)
/*  Secondary entry point  */

void Beelzebub::Secondary()
{
    Lapic::Initialize();
    //  Quickly get the local APIC initialized.

    InitializationLock.Spin();
    //  Wait for the system to initialize.

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
#endif

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

    new (&COM2) ManagedSerialPort(0x2F8);
    COM2.Initialize();

    //  Initializes the serial terminal.
    new (&initialSerialTerminal) SerialTerminal(&COM1);

    Beelzebub::Debug::DebugTerminal = &initialSerialTerminal;

    auto mbi = (multiboot_info_t *)JG_INFO_ROOT_EX->multiboot_paddr;

    new (&initialVbeTerminal) VbeTerminal((uintptr_t)mbi->framebuffer_addr, (uint16_t)mbi->framebuffer_width, (uint16_t)mbi->framebuffer_height, (uint16_t)mbi->framebuffer_pitch, (uint8_t)(mbi->framebuffer_bpp / 8));

#ifdef __BEELZEBUB__RELEASE
    Beelzebub::Debug::DebugTerminal = &initialVbeTerminal;
#endif

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

/**********
    PIT
**********/

Handle InitializePit()
{
    PitCommand pitCmd {};
    pitCmd.SetAccessMode(PitAccessMode::LowHigh);
    pitCmd.SetOperatingMode(PitOperatingMode::SquareWaveGenerator);

    Pit::SendCommand(pitCmd);
    
    uint32_t pitFreq = 100;
    Pit::SetFrequency(pitFreq);

    MainTerminal->WriteFormat(" @ %u4 Hz...", pitFreq);

    return HandleResult::Okay;
}

/***********
    ACPI
***********/

/**
 *  <summary>
 *  Initializes the ACPI tables to make them easier to use by the system.
 *  </summary>
 */
Handle InitializeAcpiTables()
{
    Handle res;

    res = Acpi::FindRsdp(MemoryManagerAmd64::IsaDmaStart + 0x0E0000
                       , MemoryManagerAmd64::IsaDmaStart + 0x100000);

    ASSERT(res.IsOkayResult()
        , "Unable to find RSDP! %H%n"
        , res);

    /*if (Acpi::RsdpPointer.GetVersion() == AcpiVersion::v1)
        msg("<[ RSDP @ %Xp, v1 ]>%n", Acpi::RsdpPointer.GetVersion1());
    else
        msg("<[ RSDP @ %Xp, v2 ]>%n", Acpi::RsdpPointer.GetVersion2());//*/

    res = Acpi::FindRsdtXsdt();

    ASSERT(res.IsOkayResult() && (Acpi::RsdtPointer != nullptr || Acpi::XsdtPointer != nullptr)
        , "Unable to find a valid RSDT or XSDT!%n"
          "RSDT @ %Xp; XSDT @ %X;%n"
          "Result = %H"
        , Acpi::RsdtPointer, Acpi::XsdtPointer, res);

    //msg("<[ XSDT @ %Xp ]>%n", Acpi::XsdtPointer);
    //msg("<[ RSDT @ %Xp ]>%n", Acpi::RsdtPointer);

    res = Acpi::FindSystemDescriptorTables();

    ASSERT(res.IsOkayResult()
        , "Failure finding system descriptor tables!%n"
          "Result = %H"
        , res);

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    MainTerminal->WriteFormat(" %us LAPIC%s, %us I/O APIC%s..."
        , Acpi::PresentLapicCount, Acpi::PresentLapicCount != 1 ? "s" : ""
        , Acpi::IoapicCount, Acpi::IoapicCount != 1 ? "s" : "");
#else
    MainTerminal->WriteFormat(" %us I/O APIC%s..."
        , Acpi::IoapicCount, Acpi::IoapicCount != 1 ? "s" : "");
#endif

    return HandleResult::Okay;
}

/***********
    APIC
***********/

Handle InitializeApic()
{
    Handle res;

    paddr_t lapicPaddr = nullpaddr;

    res = Acpi::FindLapicPaddr(lapicPaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to obtain LAPIC physical address! %H%n"
        , res);

    res = BootstrapMemoryManager.MapPage(Lapic::VirtualAddress, lapicPaddr
                                       , PageFlags::Global | PageFlags::Writable
                                       , nullptr);

    ASSERT(res.IsOkayResult()
        , "Failed to map page at %Xp (%XP) for LAPIC: %H%n"
        , Lapic::VirtualAddress, lapicPaddr, res);

    res = Lapic::Initialize();
    //  This initializes the LAPIC for the BSP.

    ASSERT(res.IsOkayResult()
        , "Failed to initialize the LAPIC?! %H%n"
        , res);

    if (Cpu::GetX2ApicMode())
        MainTerminal->Write(" Local x2APIC...");
    else
        MainTerminal->Write(" LAPIC...");

    if (Acpi::IoapicCount < 1)
    {
        MainTerminal->Write(" no I/O APIC...");

        return HandleResult::Okay;
    }
    
    uintptr_t madtEnd = (uintptr_t)Acpi::MadtPointer + Acpi::MadtPointer->Header.Length;

    uintptr_t e = (uintptr_t)Acpi::MadtPointer + sizeof(*Acpi::MadtPointer);
    for (/* nothing */; e < madtEnd; e += ((acpi_subtable_header *)e)->Length)
    {
        if (ACPI_MADT_TYPE_IO_APIC != ((acpi_subtable_header *)e)->Type)
            continue;

        auto ioapic = (acpi_madt_io_apic *)e;

        /*MainTerminal->WriteFormat("%n%*(( MADTe: I/O APIC ID-%u1 ADDR-%X4 GIB-%X4 ))"
            , (size_t)25, ioapic->Id, ioapic->Address, ioapic->GlobalIrqBase);//*/
    }

    e = (uintptr_t)Acpi::MadtPointer + sizeof(*Acpi::MadtPointer);
    for (/* nothing */; e < madtEnd; e += ((acpi_subtable_header *)e)->Length)
    {
        switch (((acpi_subtable_header *)e)->Type)
        {
        case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
            {
                auto intovr = (acpi_madt_interrupt_override *)e;

                MainTerminal->WriteFormat("%n%*(( MADTe: INT OVR BUS-%u1 SIRQ-%u1 GIRQ-%X4 IFLG-%X2 ))"
                    , (size_t)23, intovr->Bus, intovr->SourceIrq, intovr->GlobalIrq, intovr->IntiFlags);
            }
            break;

        case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
            {
                auto lanmi = (acpi_madt_local_apic_nmi *)e;

                MainTerminal->WriteFormat("%n%*(( MADTe: LA NMI PID-%u1 IFLG-%X2 LINT-%u1 ))"
                    , (size_t)34, lanmi->ProcessorId, lanmi->IntiFlags, lanmi->Lint);
            }
            break;
        }
    }

    return HandleResult::Okay;
}

/***********************
    PROCESSING UNITS
***********************/

__cold __bland Handle InitializeAp(uint32_t const lapicId
                                 , uint32_t const procId
                                 , size_t const apIndex);

/**
 *  <summary>
 *  Initializes the other processing units in the system.
 *  </summary>
 */
Handle InitializeProcessingUnits()
{
    Handle res;
    size_t apCount = 0;

    paddr_t const bootstrapPaddr = 0x1000;
    vaddr_t const bootstrapVaddr = 0x1000;
    //  This's gonna be for AP bootstrappin' code.

    res = BootstrapMemoryManager.MapPage(bootstrapVaddr, bootstrapPaddr
                                       , PageFlags::Global | PageFlags::Executable
                                       , nullptr);

    ASSERT(res.IsOkayResult()
        , "Failed to map page at %Xp (%XP) for init code: %H%n"
        , bootstrapVaddr, bootstrapPaddr, res);

    BootstrapPml4Address = BootstrapMemoryManager.Vas->Pml4Address;

    COMPILER_MEMORY_BARRIER();
    //  We need to make sure that the PML4 address is copied over.

    memcpy((void *)bootstrapVaddr, &ApBootstrapBegin, (uintptr_t)&ApBootstrapEnd - (uintptr_t)&ApBootstrapBegin);
    //  This makes sure the code can be executed by the AP.

    msg("AP bootstrap code @ %Xp:%us%n"
        , &ApBootstrapBegin, (size_t)((uintptr_t)&ApBootstrapEnd - (uintptr_t)&ApBootstrapBegin));

    KernelGdtPointer = GdtRegister::Retrieve();

    MainTerminal->WriteFormat("%n      PML4 addr: %XP, GDT addr: %Xp; BSP LAPIC ID: %u4"
        , BootstrapPml4Address, KernelGdtPointer.Pointer, Lapic::GetId());

    uintptr_t madtEnd = (uintptr_t)Acpi::MadtPointer + Acpi::MadtPointer->Header.Length;
    uintptr_t e = (uintptr_t)Acpi::MadtPointer + sizeof(*Acpi::MadtPointer);
    for (/* nothing */; e < madtEnd; e += ((acpi_subtable_header *)e)->Length)
    {
        if (ACPI_MADT_TYPE_LOCAL_APIC != ((acpi_subtable_header *)e)->Type)
            continue;

        auto lapic = (acpi_madt_local_apic *)e;

        MainTerminal->WriteFormat("%n%*(( MADTe: %s LAPIC LID-%u1 PID-%u1 F-%X4 ))"
            , (size_t)30, lapic->Id == Lapic::GetId() ? "BSP" : " AP"
            , lapic->Id, lapic->ProcessorId, lapic->LapicFlags);

        if (0 != (ACPI_MADT_ENABLED & lapic->LapicFlags)
            && lapic->Id != Lapic::GetId())
        {
            //  "Absent" LAPICs and the BSP need not be reset!

            ++apCount;

            res = InitializeAp(lapic->Id, lapic->ProcessorId, apCount);

            if unlikely(!res.IsOkayResult())
            {
                msg("Failed to initialize AP #%us (LAPIC ID %u1, processor ID %u1)"
                    ": %H%n"
                    , apCount, lapic->Id, lapic->ProcessorId, res);

                assert(false, "FAILED AP INIT!");
                //  This will only catch fire in debug mode.
            }
        }
    }

    Wait(10 * 1000);
    //  Wait a bitsy before unmapping the page.

    res = BootstrapMemoryManager.UnmapPage(bootstrapVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to unmap unneeded page at %Xp (%XP) for init code: %H%n"
        , bootstrapVaddr, bootstrapPaddr, res);

    msg("Got %us cores.", Cpu::Count.Load());

    return HandleResult::Okay;
}

Handle InitializeAp(uint32_t const lapicId
                  , uint32_t const procId
                  , size_t const apIndex)
{
    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    vaddr_t const vaddr = MemoryManagerAmd64::KernelHeapCursor.FetchAdd(PageSize);
    paddr_t const paddr = Domain0.PhysicalAllocator->AllocatePage(desc);
    //  Stack page.

    assert_or(paddr != nullpaddr && desc != nullptr
        , "Unable to allocate a physical page for stack of AP #%us"
          " (LAPIC ID %u4, processor ID %u4)!"
        , apIndex, lapicId, procId)
    {
        return HandleResult::OutOfMemory;
    }

    res = BootstrapMemoryManager.MapPage(vaddr, paddr, PageFlags::Global | PageFlags::Writable, desc);

    assert_or(res.IsOkayResult()
        , "Failed to map page at %Xp (%XP) for stack of AP #%us"
          " (LAPIC ID %u4, processor ID %u4): %H."
        , vaddr, paddr
        , apIndex, lapicId, procId
        , res)
    {
        return res;
    }

    ApStackTopPointer = vaddr + PageSize;
    ApInitializationLock = 1;

    msg("Stack for AP #%us is at %Xp (%Xp, %XP).%n", apIndex, ApStackTopPointer, vaddr, paddr);

    LapicIcr initIcr = LapicIcr(0)
    .SetDeliveryMode(InterruptDeliveryModes::Init)
    .SetDestinationShorthand(IcrDestinationShorthand::None)
    .SetAssert(true)
    .SetDestination(lapicId);

    Lapic::SendIpi(initIcr);

    //msg("Send INIT IPI to %u4.%n", lapicId);

    Wait(10 * 1000);
    //  Much more than the recommended amount, but this may be handy for busy
    //  virtualized environments.

    LapicIcr startupIcr = LapicIcr(0)
    .SetDeliveryMode(InterruptDeliveryModes::StartUp)
    .SetDestinationShorthand(IcrDestinationShorthand::None)
    .SetAssert(true)
    .SetVector(0x1000 >> 12)
    .SetDestination(lapicId);

    Lapic::SendIpi(startupIcr);

    //msg("Send first startup IPI to %u4.%n", lapicId);

    Wait(10 * 1000);

    if (ApInitializationLock != 0)
    {
        //  It should be ready. Let's try again.

        Lapic::SendIpi(startupIcr);

        //msg("Send second startup IPI to %u4.%n", lapicId);

        Wait(1000 * 1000);

        if (ApInitializationLock != 0)
            return HandleResult::Timeout;
    }

    return HandleResult::Okay;
}
