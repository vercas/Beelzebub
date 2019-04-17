/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#include "kernel.hpp"
#include "entry.h"
#include "common.hpp"
#include "global_options.hpp"
#include "utils/unit_tests.hpp"
#include "lock_elision.hpp"
#include "watchdog.hpp"
#include "djinn.arc.hpp"

#include "terminals/djinn.hpp"
#include "terminals/serial.hpp"
#include "terminals/vbe.hpp"
#include "multiboot.h"
#include "utils/port.parse.hpp"

#include "system/rtc.hpp"
#include "system/cpu.hpp"
#include "system/fpu.hpp"
#include "execution/thread_init.hpp"
#include "execution/extended_states.hpp"
#include "execution/runtime64.hpp"
#include "execution.hpp"

#include "irqs.hpp"
#include "system/acpi.hpp"
#include "system/debug.registers.hpp"
#include "system/interrupt_controllers/lapic.hpp"
#include "system/interrupt_controllers/ioapic.hpp"
#include "system/interrupt_controllers/pic.hpp"
#include "system/io_ports.hpp"
#include "system/syscalls.hpp"
#include "system/timers/pit.hpp"
#include "system/timers/apic.timer.hpp"

#ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_JEMALLOC
#include <beel/jemalloc.h>
#endif

#include "memory/vmm.arc.hpp"

#include "ap_bootstrap.hpp"

#include "utils/wait.hpp"

#include "all.tests.hpp"
#include <beel/sync/barrier.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;
using namespace Beelzebub::System::Timers;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::Utils;

/*  Synchronization  */

//SmpBarrier InitializationBarrier {};

static SmpLock InitializationLock;
static SmpLock TerminalMessageLock;
static TerminalBase * InitTerminal = nullptr;

static Barrier InitBarrier;

/*  System Globals  */

TerminalBase * Beelzebub::MainTerminal = nullptr;
MainTerminalInterfaces Beelzebub::MainTerminalInterface;

bool Beelzebub::Scheduling = false;
bool Beelzebub::CpuDataSetUp = false;

Process Beelzebub::BootstrapProcess;
Thread Beelzebub::BootstrapThread;

Domain Beelzebub::Domain0;

CpuId Beelzebub::BootstrapCpuid;

/**********************************
    System Initialization Steps
**********************************/

static bool MainShouldElideLocks = false;

/**********************
    PROTO TERMINALS
**********************/

DjinnTerminal initialDjinnTerminal;
SerialTerminal initialSerialTerminal;
VbeTerminal initialVbeTerminal;

__startup TerminalBase * InitializeProtoVbeTerminal()
{
    auto mbi = (multiboot_info_t *)JG_INFO_ROOT_EX->multiboot_paddr;

    new (&initialVbeTerminal) VbeTerminal((uintptr_t)mbi->framebuffer_addr, (uint16_t)mbi->framebuffer_width, (uint16_t)mbi->framebuffer_height, (uint16_t)mbi->framebuffer_pitch, (uint8_t)(mbi->framebuffer_bpp / 8));

    MainTerminalInterface = MainTerminalInterfaces::VBE;

    return &initialVbeTerminal;
}

__startup TerminalBase * InitializeTerminalProto()
{
    //  TODO: Properly retrieve these addresses.

    new (&COM1) ManagedSerialPort(0x03F8);
    new (&COM2) ManagedSerialPort(0x02F8);
    new (&COM3) ManagedSerialPort(0x03E8);
    new (&COM4) ManagedSerialPort(0x02E8);

    if (CMDO_Term.ParsingResult.IsValid())
    {
        auto ppr = ParsePort(CMDO_Term.StringValue);

        switch (ppr.Error)
        {
    #define CASE_COM(n) \
        case PortParseResult::COM##n:                                           \
            COM##n.Initialize();                                                \
            new (&initialSerialTerminal) SerialTerminal(&COM##n);               \
                                                                                \
            if (COM##n.Type != SerialPortType::Disconnected)                    \
            {                                                                   \
                MainTerminalInterface = MainTerminalInterfaces::COM##n;         \
                return &initialSerialTerminal;                                  \
            }                                                                   \
            break;
        CASE_COM(1)
        CASE_COM(2)
        CASE_COM(3)
        CASE_COM(4)
    #undef CASE_COM
        case PortParseResult::SpecificPort:
            //  Specifically unsupported right now. Maybe later, a PCI device can be referenced instead..?
        default:
            //  Other cases are handled later.
            break;
        }

        InitializeProtoVbeTerminal();

        switch (ppr.Error)
        {
        case PortParseResult::Vbe:
            return &initialVbeTerminal;

        case PortParseResult::SpecificPort:
            initialVbeTerminal << "Specific port values are not supported yet." << EndLine;
            break;

        case PortParseResult::Ethernet:
            initialVbeTerminal << "No terminal over Ethernet protocol supported." << EndLine;
            break;

        case PortParseResult::COM1: case PortParseResult::COM2: case PortParseResult::COM3: case PortParseResult::COM4:
            initialVbeTerminal << "Selected COM interface (" << (int)ppr.Error << ") is unuseable:" << EndLine
                << &COM1 << EndLine << &COM2 << EndLine << &COM3 << EndLine << &COM4 << EndLine;
            break;

        case PortParseResult::COM1Base64: case PortParseResult::COM2Base64:
        case PortParseResult::COM3Base64: case PortParseResult::COM4Base64:
            initialVbeTerminal << "Base64-encoded serial ports cannot be used as terminal." << EndLine;
            break;

        default:
            initialVbeTerminal << "Error parsing terminal command-line option: " << CMDO_Term.StringValue << EndLine;
            break;
        }
    }
    else
        InitializeProtoVbeTerminal();

    // msg("VM: %Xp; W: %u2, H: %u2, P: %u2; BPP: %u1.%n"
    //     , (uintptr_t)mbi->framebuffer_addr
    //     , (uint16_t)mbi->framebuffer_width, (uint16_t)mbi->framebuffer_height
    //     , (uint16_t)mbi->framebuffer_pitch, (uint8_t)mbi->framebuffer_bpp);

    // msg(" vbe_control_info: %X4%n", mbi->vbe_control_info);
    // msg(" vbe_mode_info: %X4%n", mbi->vbe_mode_info);
    // msg(" vbe_mode: %X4%n", mbi->vbe_mode);
    // msg(" vbe_interface_seg: %X4%n", mbi->vbe_interface_seg);
    // msg(" vbe_interface_off: %X2%n", mbi->vbe_interface_off);
    // msg(" vbe_interface_len: %X2%n", mbi->vbe_interface_len);

    return &initialVbeTerminal;
}

/************************************
    KERNEL COMMAND-LINE ARGUMENTS
************************************/

__startup void MainParseKernelArguments()
{
    Handle res = ParseKernelArguments();

    if (res.IsOkayResult())
    {
        res = InitializeTestFlags();

        if (res != HandleResult::Okay)
            FAIL("Failed to initialize test flags (%H; \"%s\"): %H"
                , CMDO_Tests.ParsingResult
                , CMDO_Tests.ParsingResult.IsOkayResult()
                    ? CMDO_Tests.StringValue
                    : "NO VALUE"
                , res);
    }
    else
        FAIL("Failed to parse kernel command-line arguments: %H", res);
}

/******************
    NICE THINGS
******************/

__startup void WriteWelcomeMessage()
{
    char const * const msgL = "Welcome to Beelzebub!";
    char const * const msgR = " (c) 2015 Alexandru-Mihai Maftei";

    if (MainTerminal->Capabilities->CanGetSize
        && MainTerminal->Capabilities->CanOutput
        && MainTerminal->Capabilities->CanGetOutputPosition)
    {
        TerminalWriteResult const res = MainTerminal->Write(msgL);
        TerminalCoordinates const mts = MainTerminal->GetSize();
        size_t const sizeR = 32;    //  TO BE MANUALLY CHANGED ACCORDINGLY

        if (res.End.X + sizeR <= mts.X)
        {
            //  In other words, if the right message fits after the left message
            //  in this terminal...

            MainTerminal->WriteAt(msgR, {mts.X - sizeR, res.End.Y}, sizeR);
        }
        else if (sizeR <= mts.X)
        {
            //  Doesn't fit together but fits on its own.
            //  Right message is printed onto the next line, aligned right.

            MainTerminal->WriteAt(msgR, {mts.X - sizeR, res.End.Y + 1}, sizeR);
        }
        else
        {
            //  Just won't fit. Meh.

            MainTerminal->WriteLine();
            MainTerminal->WriteLine(msgR);
        }
    }
    else
    {
        MainTerminal->Write(msgL);
        MainTerminal->WriteLine(msgR);
    }
}

/*****************
    INTERRUPTS
*****************/

__startup Handle InitializeInterrupts()
{
    size_t isrStubsSize = (size_t)(&IsrStubsEnd - &IsrStubsBegin);

    assert(isrStubsSize == Interrupts::Count
        , "ISR stubs seem to have the wrong size!");
    //  The ISR stubs must be aligned to avoid a horribly repetition.

    Handle res = Irqs::Initialize();

    // if (COM1.Type != SerialPortType::Disconnected) COM1.EnableInterrupts();
    // if (COM2.Type != SerialPortType::Disconnected) COM2.EnableInterrupts();

    return res;
}

__startup void MainInitializeInterrupts()
{
    //  Setting up basic interrupt handlers 'n stuff.
    //  Again, platform-specific.

    InitTerminal->Write("[....] Initializing interrupts...");
    Handle res = InitializeInterrupts();

    if (res.IsOkayResult())
        InitTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize interrupts: %H", res);
    }
}

/**********************
    DEBUG INTERFACE
**********************/

__startup void MainInitializeDebugInterface()
{
    //  Debug interface needs to be set up as requested in the kernel command-line arguments.
    DjinnInterfaces iface = DjinnInterfaces::None;
    Handle res;

    InitTerminal->Write("[....] Initializing debugger interface...");

    if (CMDO_Debugger.ParsingResult.IsValid())
    {
        auto ppr = ParsePort(CMDO_Debugger.StringValue);

        switch (ppr.Error)
        {
    #define CASE_COM(n) \
        case PortParseResult::COM##n##Base64:                                   \
            iface = DjinnInterfaces::COM##n##Base64;                            \
        case PortParseResult::COM##n:                                           \
            if (MainTerminalInterface == MainTerminalInterfaces::COM##n)        \
                goto conflict;                                                  \
                                                                                \
            COM##n.Initialize();                                                \
                                                                                \
            if (iface == DjinnInterfaces::None)                                 \
                iface = DjinnInterfaces::COM##n;                                \
                                                                                \
            if (COM##n.Type == SerialPortType::Disconnected)                    \
            {                                                                   \
                InitTerminal->WriteFormat(" COM" #n " not connected.\r[FAIL]%n"); \
                                                                                \
                return;                                                         \
            }                                                                   \
                                                                                \
            break;

        CASE_COM(1)
        CASE_COM(2)
        CASE_COM(3)
        CASE_COM(4)
    #undef CASE_COM

        default:
            InitTerminal->WriteFormat(" Unsupported: %i4 %s.\r[FAIL]%n", ppr.Error, CMDO_Debugger.StringValue);

            return;
        }
    }
    else
    {
        InitTerminal->WriteLine(" None specified.\r[SKIP]");

        return;
    }

    for (size_t volatile i = 0; i < 1000000000; ++i) { }

    res = InitializeDebuggerInterface(iface);

    if (res.IsOkayResult())
    {
        InitTerminal->WriteLine(" Done.\r[OKAY]");

        Debug::DebugTerminal = new (&initialDjinnTerminal) DjinnTerminal();
    }
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize debugger interface: %H", res);
    }

    return;

conflict:
    iface = DjinnInterfaces::None;
    InitTerminal->WriteLine(" Conflict with kernel terminal.\r[FAIL]");
}

/**********************
    PHYSICAL MEMORY
**********************/

static __startup void MainInitializePhysicalMemory()
{
    //  Initialize the memory by partition and allocation.
    //  Differs on IA-32 and AMD64. May tweak virtual memory in the process.

    InitTerminal->Write("[....] Initializing physical memory...");
    Handle res = InitializePhysicalMemory();

    if (res.IsOkayResult())
        InitTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize physical memory: %H", res);
    }
}

/***********
    ACPI
***********/

/**
 *  <summary>
 *  Initializes the ACPI tables to make them easier to use by the system.
 *  </summary>
 */
__startup Handle InitializeAcpiTables()
{
    Handle res;

    res = Acpi::Crawl();

    if unlikely(!res.IsOkayResult())
        return res;

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    InitTerminal->WriteFormat(" %us LAPIC%s,"
        , Acpi::PresentLapicCount, Acpi::PresentLapicCount != 1 ? "s" : "");
#endif

    InitTerminal->WriteFormat(" %us I/O APIC%s..."
        , Acpi::IoapicCount, Acpi::IoapicCount != 1 ? "s" : "");

    return HandleResult::Okay;
}

static __startup void MainInitializeAcpiTables()
{
    //  Initialize the ACPI tables for easier use.
    //  Mostly common on x86.

    InitTerminal->Write("[....] Crawling ACPI tables...");
    Handle res = InitializeAcpiTables();

    if (res.IsOkayResult())
        InitTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize the ACPI tables: %H", res);
    }
}

/*********************
    VIRTUAL MEMORY
*********************/

static __startup void MainInitializeVirtualMemory()
{
    //  Initialize the virtual memory for use by the kernel.
    //  Differs on IA-32 and AMD64, but both need the APIC remapped.

    InitTerminal->Write("[....] Initializing virtual memory...");
    Handle res = InitializeVirtualMemory();

    if (res.IsOkayResult())
    {
        InitTerminal->Write(" remapping APIC tables... ");
        res = Acpi::Remap();

        if (res.IsOkayResult())
            InitTerminal->WriteLine(" Done.\r[OKAY]");
        else
        {
            InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

            FAIL("Failed to remap APIC tables: %H", res);
        }
    }
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize virtual memory: %H", res);
    }
}

/************
    CORES
************/

static __startup void MainInitializeCores()
{
    //  Initialize the manager of processing cores.
    //  Conceptually common on x86, but implemented differently.

    InitTerminal->Write("[....] Initializing processing cores manager...");
    Handle res = Cores::Initialize(Acpi::PresentLapicCount);

    if (res.IsOkayResult())
    {
        Cores::Register();

        InitTerminal->WriteLine(" Done.\r[OKAY]");
    }
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize processing cores manager: %H", res);
    }
}

/*****************
    UNIT TESTS
*****************/

#ifdef __BEELZEBUB_SETTINGS_UNIT_TESTS
static __startup void MainRunUnitTests()
{
    //  Run all available unit tests, if asked.

    if (CMDO_UnitTests.ParsingResult.IsValid() && CMDO_UnitTests.BooleanValue)
    {
        InitTerminal->Write("[....] Running unit tests... ");

        UnitTestsReport report = RunUnitTests();

        *InitTerminal << report.SuccessCount << "/" << report.TestCount
            << " successful.\r[OKAY]" << EndLine;
    }
}
#endif

/***********
    APIC
***********/

__startup Handle InitializeApic()
{
    Handle res;

    paddr_t lapicPaddr = nullpaddr;

    res = Acpi::FindLapicPaddr(lapicPaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to obtain LAPIC physical address.")
        (res);

    res = Vmm::MapPage(&BootstrapProcess
        , Lapic::VirtualAddress
        , lapicPaddr
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryMapOptions::NoReferenceCounting);

    ASSERT(res.IsOkayResult(), "Failed to map page for LAPIC.")
        (Lapic::VirtualAddress)(lapicPaddr)(res);

    res = Lapic::Initialize();
    //  This initializes the LAPIC for the BSP.

    ASSERT(res.IsOkayResult()
        , "Failed to initialize the LAPIC.")
        (res);

    if (Lapic::X2ApicMode)
        InitTerminal->Write(" Local x2APIC...");
    else
        InitTerminal->Write(" LAPIC...");

    if (Acpi::IoapicCount < 1)
    {
        InitTerminal->Write(" no I/O APIC...");

        return HandleResult::Okay;
    }
    
    // uintptr_t madtEnd = (uintptr_t)Acpi::MadtPointer + Acpi::MadtPointer->Header.Length;

    // uintptr_t e = (uintptr_t)Acpi::MadtPointer + sizeof(*Acpi::MadtPointer);
    // for (/* nothing */; e < madtEnd; e += ((acpi_subtable_header *)e)->Length)
    // {
    //     if (ACPI_MADT_TYPE_IO_APIC != ((acpi_subtable_header *)e)->Type)
    //         continue;

    //     auto ioapic = (acpi_madt_io_apic *)e;

    //     InitTerminal->WriteFormat("%n%*(( MADTe: I/O APIC ID-%u1 ADDR-%X4 GIB-%X4 ))"
    //         , (size_t)25, ioapic->Id, ioapic->Address, ioapic->GlobalIrqBase);
    // }

    // e = (uintptr_t)Acpi::MadtPointer + sizeof(*Acpi::MadtPointer);
    // for (/* nothing */; e < madtEnd; e += ((acpi_subtable_header *)e)->Length)
    // {
    //     switch (((acpi_subtable_header *)e)->Type)
    //     {
    //     case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
    //         {
    //             auto intovr = (acpi_madt_interrupt_override *)e;

    //             InitTerminal->WriteFormat("%n%*(( MADTe: INT OVR BUS-%u1 SIRQ-%u1 GIRQ-%X4 IFLG-%X2 ))"
    //                 , (size_t)23, intovr->Bus, intovr->SourceIrq, intovr->GlobalIrq, intovr->IntiFlags);
    //         }
    //         break;

    //     case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
    //         {
    //             auto lanmi = (acpi_madt_local_apic_nmi *)e;

    //             InitTerminal->WriteFormat("%n%*(( MADTe: LA NMI PID-%u1 IFLG-%X2 LINT-%u1 ))"
    //                 , (size_t)32, lanmi->ProcessorId, lanmi->IntiFlags, lanmi->Lint);
    //         }
    //         break;
    //     }
    // }

    return HandleResult::Okay;
}

static __startup void MainInitializeApic()
{
    //  Initialize the LAPIC for the BSP and the I/O APIC.
    //  Mostly common on x86.

    InitTerminal->Write("[....] Initializing APIC...");
    Handle res = InitializeApic();

    if (res.IsOkayResult())
        InitTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize the APIC: %H", res);
    }
}

/*************
    TIMERS
*************/

__startup Handle InitializeTimers()
{
    PitCommand pitCmd {};
    pitCmd.SetAccessMode(PitAccessMode::LowHigh);
    pitCmd.SetOperatingMode(PitOperatingMode::SquareWaveGenerator);

    Pic::SetMasked(0, false);
    //  TODO: Burn.

    Pit::SendCommand(pitCmd);

    ApicTimer::Initialize(true);

    InitTerminal->WriteFormat(" APIC @ %u8 Hz...", ApicTimer::Frequency);

    Pit::SetFrequency(1000);
    //  This frequency really shouldn't stress the BSP that much, considering
    //  that the IRQ would get 2-3 million clock cycles on modern chips.

    InitTerminal->WriteFormat(" PIT @ %u4 Hz...", Pit::Frequency);

    Timer::Initialize();

    return HandleResult::Okay;
}

static __startup void MainInitializeTimers()
{
    //  Preparing the timers for basic timing.
    //  Common on x86.

    InitTerminal->Write("[....] Initializing timers...");
    Handle res = InitializeTimers();

    if (res.IsOkayResult())
        InitTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize the timers: %H", res);
    }
}

/**************
    MAILBOX
**************/

#ifdef __BEELZEBUB_SETTINGS_SMP
static __startup void MainInitializeMailbox()
{
    //  Preparing the mailbox.
    //  Common on x86.

    InitTerminal->Write("[....] Initializing mailbox...");
    
    Mailbox::Initialize();

    InitTerminal->WriteLine(" Done.\r[OKAY]");
}
#endif

static __startup void MainBootstrapThread()
{
    //  Turns the current system state into a kernel process and a main thread.

    InitTerminal->Write("[....] Initializing as bootstrap thread...");

    Handle res = InitializeBootstrapThread(&BootstrapThread, &BootstrapProcess);

    if (res.IsOkayResult())
        InitTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize main entry point as bootstrap thread: %H", res);
    }

    Cpu::SetThread(&BootstrapThread);
    Cpu::SetProcess(&BootstrapProcess);
}

/***********************
    PROCESSING UNITS
***********************/

__startup bool CheckApInitializationLock1()
{
    uint32_t volatile val = ApInitializationLock1;

    return val == 0;
}

__startup bool CheckApInitializationLock3()
{
    uint32_t volatile val = ApInitializationLock3;

    return val == 0;
}

__startup Handle InitializeAp(uint32_t const lapicId
                            , uint32_t const procId
                            ,   size_t const apIndex)
{
    Handle res;
    //  Intermediate results.

    vaddr_t vaddr = nullvaddr;

    res = Vmm::AllocatePages(nullptr
        , vsize_t(CpuStackSize)
        , MemoryAllocationOptions::Commit   | MemoryAllocationOptions::VirtualKernelHeap
        | MemoryAllocationOptions::GuardLow | MemoryAllocationOptions::GuardHigh
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::ThreadStack
        , vaddr);

    assert_or(res.IsOkayResult()
        , "Failed to allocate stack of AP #%us"
          " (LAPIC ID %u4, processor ID %u4): %H."
        , apIndex, lapicId, procId
        , res)
    {
        return res;
    }

    ApStackTopPointer = vaddr.Value + CpuStackSize;
    ApInitializationLock1 = ApInitializationLock2 = ApInitializationLock3 = 1;

    LapicIcr initIcr = LapicIcr(0)
    .SetDeliveryMode(InterruptDeliveryModes::Init)
    .SetDestinationShorthand(IcrDestinationShorthand::None)
    .SetAssert(true)
    .SetDestination(lapicId);

    Lapic::SendIpi(initIcr);

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

    if (!Wait(10 * 1000, &CheckApInitializationLock1))
    {
        //  It should be ready. Let's try again.

        Lapic::SendIpi(startupIcr);

        if (!Wait(1000 * 1000, &CheckApInitializationLock1))
            return HandleResult::Timeout;
    }

    //  If this point was reached, the AP has begun the handshake.

    ApInitializationLock2 = 0;
    //  Lower the entry barrier for the AP.

    if likely(Wait(3 * 1000 * 1000, &CheckApInitializationLock3))
        return HandleResult::Okay;
    //  Now the AP must acknowledge passing the barrier before the BSP can
    //  proceed. The timeout here is just symbolic and this should succeed at
    //  the first or second calls of the predicate.
    
    return HandleResult::Timeout;
}

__startup Handle InitializeProcessingUnits()
{
    Handle res;
    size_t apCount = 0;

    paddr_t const bootstrapPaddr { 0x1000 };
    vaddr_t const bootstrapVaddr { 0x1000 };
    //  This's gonna be for AP bootstrappin' code.

    res = Vmm::MapPage(&BootstrapProcess, bootstrapVaddr, bootstrapPaddr
        , MemoryFlags::Global | MemoryFlags::Executable | MemoryFlags::Writable
        , MemoryMapOptions::NoReferenceCounting);

    ASSERT(res.IsOkayResult()
        , "Failed to map page for AP bootstrap code.")
        (bootstrapVaddr)(bootstrapPaddr)(res);

    res = Vmm::InvalidatePage(nullptr, bootstrapVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to invalidate page for AP bootstrap code.")
        (bootstrapVaddr)(bootstrapPaddr)(res);

    BootstrapPml4Address = BootstrapProcess.PagingTable;

    COMPILER_MEMORY_BARRIER();
    //  Needed to make sure that the PML4 address is copied over.

    memcpy((void *)bootstrapVaddr, &ApBootstrapBegin, (uintptr_t)&ApBootstrapEnd - (uintptr_t)&ApBootstrapBegin);
    //  This makes sure the code can be executed by the AP.

    KernelGdtPointer = GdtRegister::Retrieve();

    BREAKPOINT_SET_AUX((int volatile *)((uintptr_t)&ApBreakpointCookie - (uintptr_t)&ApBootstrapBegin + bootstrapVaddr.Value));
    InterruptState const int_cookie = InterruptState::Enable();

    // InitTerminal->WriteFormat("%n      PML4 addr: %XP, GDT addr: %Xp; BSP LAPIC ID: %u4"
    //     , BootstrapPml4Address, KernelGdtPointer.Pointer, Lapic::GetId());

    uintptr_t madtEnd = (uintptr_t)Acpi::MadtPointer + Acpi::MadtPointer->Header.Length;
    uintptr_t e = (uintptr_t)Acpi::MadtPointer + sizeof(*Acpi::MadtPointer);
    for (/* nothing */; e < madtEnd; e += ((acpi_subtable_header *)e)->Length)
    {
        if (ACPI_MADT_TYPE_LOCAL_APIC != ((acpi_subtable_header *)e)->Type)
            continue;

        auto lapic = (acpi_madt_local_apic *)e;

        // InitTerminal->WriteFormat("%n%*(( MADTe: %s LAPIC LID-%u1 PID-%u1 F-%X4 ))"
        //     , (size_t)30, lapic->Id == Lapic::GetId() ? "BSP" : " AP"
        //     , lapic->Id, lapic->ProcessorId, lapic->LapicFlags);

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

    int_cookie.Restore();
    BREAKPOINT_SET_AUX(nullptr);

    res = Vmm::UnmapPage(&BootstrapProcess, bootstrapVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to unmap page containing AP boostrap code.")
        (bootstrapVaddr)(bootstrapPaddr)(res);

    CpuDataSetUp = true;
    //  Let the kernel know that CPU data is available for use.

    return HandleResult::Okay;
}

static __startup void MainInitializeExtraCpus()
{
    //  Initialize the other processing units in the system.
    //  Mostly common on x86, but the executed code differs by arch.

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    //  Note: If SMP is disabled by build, nothing is done, and only the BSP is
    //  used.

    InitTerminal->WriteLine("[SKIP] Kernel was build with SMP disabled. Other processing units ignored.");
#else
    MainShouldElideLocks = false;

    if (CMDO_SmpEnable.ParsingResult.IsValid() && !CMDO_SmpEnable.BooleanValue)
    {
        InitTerminal->WriteLine("[SKIP] Extra processing units ignored as indicated in arguments.");

        CpuDataSetUp = MainShouldElideLocks = true;
        //  Let the kernel know that CPU data is available for use, and elide
        //  useless locks.
    }
    else if (Acpi::PresentLapicCount > 1)
    {
        InitTerminal->Write("[....] Initializing extra processing units...");
        Handle res = InitializeProcessingUnits();

        if (res.IsOkayResult())
            InitTerminal->WriteLine(" Done.\r[OKAY]");
        else
        {
            InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

            FAIL("Failed to initialize the extra processing units: %H", res);
        }
    }
    else
    {
        InitTerminal->WriteLine("[SKIP] No extra processing units available.");

        CpuDataSetUp = MainShouldElideLocks = true;
        //  Once again...
    }
#endif
}

static __startup void MainElideLocks()
{
#ifdef __BEELZEBUB__TEST_LOCK_ELISION
    if (CHECK_TEST(LOCK_ELISION))
    {
        InitTerminal->WriteLine("[TEST] Testing lock elision...");

        TestLockElision();
    }
#endif

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    //  Elide lock operations on unicore systems.
    //  Mainly common.
   #ifdef __BEELZEBUB__TEST_LOCK_ELISION
    if (MainShouldElideLocks && !(CHECK_TEST(LOCK_ELISION)))
   #else
    if (MainShouldElideLocks)
   #endif
    {
        InitTerminal->Write("[....] Eliding locks...");
        Handle res = ElideLocks();

        if (res.IsOkayResult())
            InitTerminal->WriteLine(" Done.\r[OKAY]");
        else
        {
            InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

            FAIL("Failed to elide locks: %H", res);
        }
    }
#endif
}

static __startup void MainInitializeBootModules()
{
    //  Initialize the modules loaded by the bootloader with the kernel.
    //  Mostly common.

    InitTerminal->Write("[....] Initializing boot modules...");
    Handle res = InitializeModules();

    if (res.IsOkayResult())
    {
        // if (KernelImage != nullptr)
            InitTerminal->WriteLine(" Done.\r[OKAY]");
        // else
        // {
        //     InitTerminal->WriteFormat(" Fail! No kernel image found.\r[FAIL]%n");

        //     FAIL("Kernel image hasn't been found!");
        // }
    }
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize modules: %H", res);
    }
}

static __startup void MainInitializeRuntimeLibraries()
{
    //  Initialize the modules loaded with the kernel.
    //  Mostly common.

    InitTerminal->Write("[....] Initializing runtime libraries...");

#if   defined(__BEELZEBUB__ARCH_AMD64)
    Handle res = Runtime64::Initialize();

    if (res.IsOkayResult())
        InitTerminal->Write(" 64-bit...");
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize 64-bit runtime: %H", res);
    }
#endif

    InitTerminal->WriteLine(" Done.\r[OKAY]");
    
    //  TODO: The 32-bit subsystem should be handled by a kernel module.
}

static __startup void MainInitializeFpu()
{
    //  Initialize the extended thread states manager.
    //  Mostly common.

    Fpu::InitializeMain();
    //  Meh.

    if (Fpu::StateSize != 0)
    {
        InitTerminal->Write("[....] Initializing extended thread states...");
        Handle res = ExtendedStates::Initialize(Fpu::StateSize, Fpu::StateAlignment);

        if (res.IsOkayResult())
        {
            InitTerminal->Write(" Allocating template state...");

            void * templateState;

            res = ExtendedStates::AllocateTemplate(templateState);

            if (res.IsOkayResult())
                InitTerminal->WriteLine(" Done.\r[OKAY]");
            else
            {
                InitTerminal->WriteLine("\r[FAIL]%n");
                InitTerminal->WriteLine("       Fail! Could not allocate template state.");

                FAIL("Failed to allocate template extended thread state: %H", res);
            }

            Fpu::SaveState(templateState);

            Cpu::SetCr0(Cpu::GetCr0().SetTaskSwitched(true));
        }
        else
        {
            InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

            FAIL("Failed to initialize extended thread states: %H", res);
        }
    }
    else
    {
        InitTerminal->WriteLine("[SKIP] Extended thread states not needed by present CPU features.");
    }
}

static __startup void MainInitializeSyscalls()
{
    InitTerminal->Write("[....] Initializing syscalls...");
    Syscall::Initialize();
    InitTerminal->WriteLine(" Done.\r[OKAY]");
}

static __startup void MainInitializeKernelModules()
{
    //  Prepare the kernel for loading modules into itself.
    //  Pretty much architecture-agnostic, at this point at least.

    InitTerminal->Write("[....] Initializing kernel modules...");
    Handle res = Modules::Initialize();

    if (res.IsOkayResult())
        InitTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        InitTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        FAIL("Failed to initialize kernel modules: %H", res);
    }
}

/*********************
    MAIN TERMINALS
*********************/

__startup TerminalBase * InitializeTerminalMain()
{
    switch (MainTerminalInterface)
    {
    case MainTerminalInterfaces::VBE:
        return &initialVbeTerminal;
    default:
        return &initialSerialTerminal;
    }

    //  TODO: Maybe get rid of this? It was added because of some very twisteed
    //  assumptions, way too early in the development process.
}

static __startup void MainInitializeMainTerminal()
{
    //  Upgrade the terminal to a more capable and useful one.
    //  Yet again, platform-specific.

    InitTerminal->Write("[....] Initializing main terminal...");

    TerminalBase * const secondaryTerminal = InitializeTerminalMain();

    InitTerminal->WriteLine(" Done.\r[OKAY]");
    InitTerminal->WriteLine("Switching over.");

    MainTerminal = secondaryTerminal;
}

/*******************
    ENTRY POINTS
*******************/

/*  Main entry point  */

void Beelzebub::Main()
{
    Handle res;
    //  Used for intermediary results.

    BootstrapCpuid = CpuId();
    BootstrapCpuid.Initialize();
    //  This is required to page all the available memory.

    new (&Domain0) Domain();
    //  Initialize domain 0. Make sure it's not in a possibly-invalid state.

    InitializationLock.Reset();
    TerminalMessageLock.Reset();
    Domain0.GdtLock.Reset();
    //  Make sure these are clear.

    Domain0.Gdt = GdtRegister::Retrieve();

    // for (size_t volatile i = 1000000000000; i > 0; --i) DO_NOTHING();

    //  Step 0 is parsing the command-line arguments given to the kernel, so it knows how to operate.
    MainParseKernelArguments();

    //  Basic terminal(s) are initialized as soon as possible.
    MainTerminal = InitializeTerminalProto();
    InitTerminal = MainTerminal;

    WriteWelcomeMessage();

    // MSG("Stack pointer in Beelzebub::Main is %Xp.%n", GetCurrentStackPointer());

    MainInitializeDebugInterface();
    MainInitializeInterrupts();
    //  These two steps need to be finished as quickly as possible.

    Rtc::Read();
    DEBUG_TERM << "Boot time: " << Rtc::Year << '-' << Rtc::Month << '-' << Rtc::Day
        << ' ' << Rtc::Hours << ':' << Rtc::Minutes << ':' << Rtc::Seconds << EndLine;

    if (Debug::DebugTerminal != nullptr)
    {
        BootstrapCpuid.PrintToTerminal(Debug::DebugTerminal);
        msg("%n");

        InitTerminal = Debug::DebugTerminal;
    }

    // MSGEX("Test {0} {1} {2} {2} {0} {1} {1} {0:bit}.\n", true, -124, "rada");

    MainInitializePhysicalMemory();
    MainInitializeAcpiTables();
    MainInitializeVirtualMemory();
    MainInitializeBootModules();

    //  This should really be done under a lock.
    InitializationLock.Acquire();

    MainInitializeCores();

    DebugRegisters::Initialize();

#ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_JEMALLOC
    InitializeJemalloc(true);
#endif

#ifdef __BEELZEBUB_SETTINGS_UNIT_TESTS
    MainRunUnitTests();
#endif

    MainInitializeApic();
    MainInitializeTimers();

#ifdef __BEELZEBUB_SETTINGS_SMP
    MainInitializeMailbox();
#endif

    MainBootstrapThread();

    InitializeExecutionData();

#ifdef __BEELZEBUB__TEST_STACKINT
    if (CHECK_TEST(STACKINT))
        StackIntTestBarrier.Reset(Cores::GetCount());
#endif

#if     defined(__BEELZEBUB__TEST_RW_SPINLOCK) && defined(__BEELZEBUB_SETTINGS_SMP)
    if (Cores::GetCount() > 1 && CHECK_TEST(RW_SPINLOCK))
        RwSpinlockTestBarrier.Reset(Cores::GetCount());
#endif

#if     defined(__BEELZEBUB__TEST_RW_TICKETLOCK) && defined(__BEELZEBUB_SETTINGS_SMP)
    if (Cores::GetCount() > 1 && CHECK_TEST(RW_TICKETLOCK))
        RwTicketLockTestBarrier.Reset(Cores::GetCount());
#endif

#if defined(__BEELZEBUB_SETTINGS_SMP) && defined(__BEELZEBUB__TEST_MAILBOX)
    if (CHECK_TEST(MAILBOX))
        MailboxTestBarrier.Reset(Cores::GetCount());
#endif

#ifdef __BEELZEBUB__TEST_PMM
    if (CHECK_TEST(PMM))
        PmmTestBarrier.Reset(Cores::GetCount());
#endif

#ifdef __BEELZEBUB__TEST_VMM
    if (CHECK_TEST(VMM))
        VmmTestBarrier.Reset(Cores::GetCount());
#endif

#ifdef __BEELZEBUB__TEST_OBJA
    if (CHECK_TEST(OBJA))
        ObjectAllocatorTestBarrier.Reset(Cores::GetCount());
#endif

#if defined(__BEELZEBUB__TEST_MALLOC) && !defined(__BEELZEBUB_SETTINGS_KRNDYNALLOC_NONE)
    if (CHECK_TEST(MALLOC))
        MallocTestBarrier.Reset(Cores::GetCount());
#endif

    MainInitializeExtraCpus();
    // MainElideLocks();

    MainInitializeRuntimeLibraries();

    MainInitializeFpu();
    MainInitializeSyscalls();
    MainInitializeKernelModules();

    MainInitializeMainTerminal();

    //  Permit other processors to initialize themselves.
    InitTerminal->WriteLine("--  Initialization complete! --");
    
    InitBarrier.Reset(Cores::GetCount());

    InitializationLock.Release();

    InitBarrier.Reach();

    Scheduling = true;

    Interrupts::Enable();

    // MSG_("Stack pointer in Beelzebub::Main post init is %Xp.%n", GetCurrentStackPointer());

#ifdef __BEELZEBUB__TEST_METAP
    if (CHECK_TEST(METAP))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] Metaprgoramming facilities...");

        TestMetaprogramming();
    }
#endif

#ifdef __BEELZEBUB__TEST_EXCP
    if (CHECK_TEST(EXCP))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] Exceptions...");
        MSG_("Starting exceptions test.%n");

        TestExceptions();
    }
#endif

#ifdef __BEELZEBUB__TEST_STR
    if (CHECK_TEST(STR))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] string.h implementation...");
        MSG_("Starting string library test.%n");

        TestStringLibrary();
    }
#endif

#ifdef __BEELZEBUB__TEST_CMDO
    if (CHECK_TEST(CMDO))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] Command-line options parsing...");

        TestCmdo();
    }
#endif

#ifdef __BEELZEBUB__TEST_TIMER
    if (CHECK_TEST(TIMER))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] Generic timer...");

        TestTimer();
    }
#endif

#ifdef __BEELZEBUB__TEST_AVL_TREE
    if (CHECK_TEST(AVL_TREE))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] AVL trees...");

        TestAvlTree();
    }
#endif

#ifdef __BEELZEBUB__TEST_VAS
    if (CHECK_TEST(VAS))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] VAS implementation...");

        TestVas();
    }
#endif

#ifdef __BEELZEBUB__TEST_KMOD
    if (CHECK_TEST(KMOD))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] A kernel module...");

        TestKmod();
    }
#endif

#ifdef __BEELZEBUB__TEST_TERMINAL
    if (CHECK_TEST(TERMINAL))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] Terminal implementation(s)...");

        TestTerminal();
    }
#endif

#ifdef __BEELZEBUB__TEST_MT
    if (CHECK_TEST(MT))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] Starting multitasking test...");

        StartMultitaskingTest();
    }
#endif

#ifdef __BEELZEBUB__TEST_INTERRUPT_LATENCY
    if (CHECK_TEST(INT_LAT))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] Interrupt latency...");

        TestInterruptLatency();
    }
#endif

#ifdef __BEELZEBUB__TEST_BIGINT
    if (CHECK_TEST(BIGINT))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] Big integer implementation...");

        TestBigInt();
    }
#endif

#ifdef __BEELZEBUB__TEST_FPU
    if (CHECK_TEST(FPU))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteLine("[TEST] FPU and SSE...");

        TestFpu();
    }
#endif

#ifdef __BEELZEBUB__TEST_STACKINT
    if (CHECK_TEST(STACKINT))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing stack integrity.%n", Cpu::GetData()->Index);

        TestStackIntegrity(true);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished stack integrity test.%n", Cpu::GetData()->Index);
    }
#endif

#if     defined(__BEELZEBUB__TEST_RW_SPINLOCK) && defined(__BEELZEBUB_SETTINGS_SMP)
    if (Cores::GetCount() > 1 && CHECK_TEST(RW_SPINLOCK))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing R/W spinlock.%n", Cpu::GetData()->Index);

        TestRwSpinlock(true);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished R/W spinlock test.%n", Cpu::GetData()->Index);
    }
#endif

#if     defined(__BEELZEBUB__TEST_RW_TICKETLOCK) && defined(__BEELZEBUB_SETTINGS_SMP)
    if (Cores::GetCount() > 1 && CHECK_TEST(RW_TICKETLOCK))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing R/W ticket lock.%n", Cpu::GetData()->Index);

        TestRwTicketLock(true);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished R/W ticket lock test.%n", Cpu::GetData()->Index);
    }
#endif

#if defined(__BEELZEBUB_SETTINGS_SMP) && defined(__BEELZEBUB__TEST_MAILBOX)
    if (CHECK_TEST(MAILBOX))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing mailbox.%n", Cpu::GetData()->Index);

        TestMailbox(true);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished mailbox test.%n", Cpu::GetData()->Index);
    }
#endif

#ifdef __BEELZEBUB__TEST_PMM
    if (CHECK_TEST(PMM))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing physical memory manager.%n", Cpu::GetData()->Index);

        TestPmm(true);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished PMM test.%n", Cpu::GetData()->Index);
    }
#endif

#ifdef __BEELZEBUB__TEST_VMM
    if (CHECK_TEST(VMM))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing virtual memory manager.%n", Cpu::GetData()->Index);

        TestVmm(true);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished VMM test.%n", Cpu::GetData()->Index);
    }
#endif

#ifdef __BEELZEBUB__TEST_OBJA
    if (CHECK_TEST(OBJA))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing fixed-sized object allocator.%n", Cpu::GetData()->Index);

        TestObjectAllocator(true);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished object allocator test.%n", Cpu::GetData()->Index);
    }
#endif

#if defined(__BEELZEBUB__TEST_MALLOC) && !defined(__BEELZEBUB_SETTINGS_KRNDYNALLOC_NONE)
    if (CHECK_TEST(MALLOC))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing dynamic allocator.%n", Cpu::GetData()->Index);

        TestMalloc(true);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished dynamic allocator test.%n", Cpu::GetData()->Index);
    }
#endif

    //  Allow the CPU to rest.
    while (true) if (CpuInstructions::CanHalt) CpuInstructions::Halt();
}

#if   defined(__BEELZEBUB_SETTINGS_SMP)
/*  Secondary entry point  */

void Beelzebub::Secondary()
{
    InitializationLock.Acquire();

    MSG_("Initializing AP... %W");

    Interrupts::Register.Activate();
    //  Very important for detecting errors ASAP.

    MSG_("Activated IDT... %W");

    Vmm::Switch(nullptr, &BootstrapProcess);
    //  Perfectly valid solution. Just to make sure.

    MSG_("Switched to bootstrap process... %W");

    ++BootstrapProcess.ActiveCoreCount;
    //  Leave the process in a valid state.

    MSG_("blergh... %W");

    Cores::Register();
    //  Register the core with the core manager.

    MSG_("Registered core #%us... %W", Cpu::GetData()->Index);

    Lapic::Initialize();
    //  Quickly get the local APIC initialized.

    MSG_("Initialized LAPIC... %W");

    DebugRegisters::Initialize();
    //  Debug registers are always handy.

    MSG_("Initialized debug registers... %W");

    Syscall::Initialize();
    //  And syscalls.

    MSG_("Initialized syscalls... %W");

    ApicTimer::Initialize(false);
    Timer::Initialize();
    //  And timers.

    MSG_("Initialized timers... %W");

    // InitializationLock.Spin();
    //  Wait for the system to initialize.

    Fpu::InitializeSecondary();
    //  Meh...

    MSG_("Initialized FPU... %W");

    Interrupts::Enable();
    //  Enable interrupts, this core is almost ready.

    MSG_("Enabled interrupts... %W");

    Mailbox::Initialize();
    //  And the mailbox. This one needs interrupts enabled.

    MSG_("Initialized mailbox!%n%W");

    // Watchdog::Initialize();
    // //  Sadly needed.

    InitializationLock.Release();

    InitBarrier.Reach();

#ifdef __BEELZEBUB__TEST_STACKINT
    if (CHECK_TEST(STACKINT))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing stack integrity.%n", Cpu::GetData()->Index);

        TestStackIntegrity(false);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished stack integrity test.%n", Cpu::GetData()->Index);
    }
#endif

#ifdef __BEELZEBUB__TEST_RW_SPINLOCK
    if (CHECK_TEST(RW_SPINLOCK))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing R/W spinlock.%n", Cpu::GetData()->Index);
        
        TestRwSpinlock(false);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished R/W spinlock test.%n", Cpu::GetData()->Index);
    }
#endif

#if     defined(__BEELZEBUB__TEST_RW_TICKETLOCK) && defined(__BEELZEBUB_SETTINGS_SMP)
    if (Cores::GetCount() > 1 && CHECK_TEST(RW_TICKETLOCK))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing R/W ticket lock.%n", Cpu::GetData()->Index);

        TestRwTicketLock(false);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished R/W ticket lock test.%n", Cpu::GetData()->Index);
    }
#endif

#if defined(__BEELZEBUB_SETTINGS_SMP) && defined(__BEELZEBUB__TEST_MAILBOX)
    if (CHECK_TEST(MAILBOX))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing mailbox.%n", Cpu::GetData()->Index);

        TestMailbox(false);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished mailbox test.%n", Cpu::GetData()->Index);
    }
#endif

#ifdef __BEELZEBUB__TEST_PMM
    if (CHECK_TEST(PMM))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing physical memory manager.%n", Cpu::GetData()->Index);
        
        TestPmm(false);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished PMM test.%n", Cpu::GetData()->Index);
    }
#endif

#ifdef __BEELZEBUB__TEST_VMM
    if (CHECK_TEST(VMM))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing virtual memory manager.%n", Cpu::GetData()->Index);
        
        TestVmm(false);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished VMM test.%n", Cpu::GetData()->Index);
    }
#endif

#ifdef __BEELZEBUB__TEST_OBJA
    if (CHECK_TEST(OBJA))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing fixed-sized object allocator.%n", Cpu::GetData()->Index);

        TestObjectAllocator(false);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished object allocator test.%n", Cpu::GetData()->Index);
    }
#endif

#if defined(__BEELZEBUB__TEST_MALLOC) && !defined(__BEELZEBUB_SETTINGS_KRNDYNALLOC_NONE)
    if (CHECK_TEST(MALLOC))
    {
        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Testing dynamic allocator.%n", Cpu::GetData()->Index);
        
        TestMalloc(false);

        withLock (TerminalMessageLock)
            InitTerminal->WriteFormat("Core %us: Finished dynamic allocator test.%n", Cpu::GetData()->Index);
    }
#endif

    //  Allow the CPU to rest.
    while (true) if (CpuInstructions::CanHalt) CpuInstructions::Halt();
}
#endif
