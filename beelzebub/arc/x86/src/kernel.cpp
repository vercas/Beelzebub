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

#include <kernel.hpp>
#include <entry.h>
#include <global_options.hpp>
#include <utils/unit_tests.hpp>
#include <lock_elision.hpp>

#include <terminals/serial.hpp>
#include <terminals/vbe.hpp>
#include <multiboot.h>

#include <system/rtc.hpp>
#include <system/cpu.hpp>
#include <system/fpu.hpp>
#include <execution/thread_init.hpp>
#include <execution/extended_states.hpp>
#include <execution/runtime64.hpp>

#include <system/exceptions.hpp>
#include <system/interrupt_controllers/pic.hpp>
#include <system/interrupt_controllers/lapic.hpp>
#include <system/interrupt_controllers/ioapic.hpp>
#include <system/timers/pit.hpp>
#include <system/timers/apic.timer.hpp>
#include <system/syscalls.hpp>
#include <modules.hpp>

#include <memory/vmm.hpp>
#include <memory/vmm.arc.hpp>
#include <memory/pmm.hpp>
#include <system/acpi.hpp>

#include <ap_bootstrap.hpp>

#include <synchronization/spinlock.hpp>

#include <utils/wait.hpp>
#include <string.h>

#include <_print/gdt.hpp>
#include <debug.hpp>

#ifdef __BEELZEBUB__TEST_STACKINT
#include <tests/stack_integrity.hpp>
#endif

#ifdef __BEELZEBUB__TEST_METAP
#include <tests/meta.hpp>
#endif

#ifdef __BEELZEBUB__TEST_EXCP
#include <tests/exceptions.hpp>
#endif

#ifdef __BEELZEBUB__TEST_STR
#include <tests/string.hpp>
#endif

#ifdef __BEELZEBUB__TEST_TERMINAL
#include <tests/terminal.hpp>
#endif

#ifdef __BEELZEBUB__TEST_OBJA
#include <tests/object_allocator.hpp>
#endif

#ifdef __BEELZEBUB__TEST_AVL_TREE
#include <tests/avl_tree.hpp>
#endif

#ifdef __BEELZEBUB__TEST_CMDO
#include <tests/cmdo.hpp>
#endif

#ifdef __BEELZEBUB__TEST_FPU
#include <tests/fpu.hpp>
#endif

#ifdef __BEELZEBUB__TEST_BIGINT
#include <tests/bigint.hpp>
#endif

#ifdef __BEELZEBUB__TEST_LOCK_ELISION
#include <tests/lock_elision.hpp>
#endif

#ifdef __BEELZEBUB__TEST_RW_SPINLOCK
#include <tests/rw_spinlock.hpp>
#endif

#ifdef __BEELZEBUB__TEST_VAS
#include <tests/vas.hpp>
#endif

#ifdef __BEELZEBUB__TEST_INTERRUPT_LATENCY
#include <tests/interrupt_latency.hpp>
#endif

#ifdef __BEELZEBUB__TEST_KMOD
#include <tests/kmod.hpp>
#endif

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

Spinlock<> InitializationLock;
Spinlock<> TerminalMessageLock;

/*  System Globals  */

TerminalBase * Beelzebub::MainTerminal = nullptr;
bool Beelzebub::Scheduling = false;
bool Beelzebub::CpuDataSetUp = false;

Process Beelzebub::BootstrapProcess;
Thread Beelzebub::BootstrapThread;

Domain Beelzebub::Domain0;

/**********************************
    System Initialization Steps
**********************************/

static bool MainShouldElideLocks = false;

static __startup Handle InitializeTimers();

//  These are minor initialization steps performed in the order they appear here
//  in the source code.

static __startup void MainParseKernelArguments()
{
    //  Parsing the command-line arguments given to the kernel by the bootloader.
    //  Again, platform-specific.

    MainTerminal->Write("[....] Parsing command-line arguments...");
    Handle res = ParseKernelArguments();

    if (res.IsOkayResult())
    {
        MainTerminal->Write(" And tests...");
        res = InitializeTestFlags();

        if (res.IsOkayResult())
            MainTerminal->WriteLine(" Done.\r[OKAY]");
        else
        {
            MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

            ASSERT(false, "Failed to initialize test flags (%H; \"%s\"): %H"
                , CMDO_Tests.ParsingResult
                , CMDO_Tests.ParsingResult.IsOkayResult()
                    ? CMDO_Tests.StringValue
                    : "NO VALUE"
                , res);
        }
    }
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to parse kernel command-line arguments: %H"
            , res);
    }
}

static __startup void MainInitializeInterrupts()
{
    //  Setting up basic interrupt handlers 'n stuff.
    //  Again, platform-specific.

    MainTerminal->Write("[....] Initializing interrupts...");
    Handle res = InitializeInterrupts();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize interrupts: %H"
            , res);
    }
}

static __startup void MainInitializeTimers()
{
    //  Preparing the timers for basic timing.
    //  Common on x86.

    MainTerminal->Write("[....] Initializing timers...");
    Handle res = InitializeTimers();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize the timers: %H"
            , res);
    }
}

static __startup void MainInitializePhysicalMemory()
{
    //  Initialize the memory by partition and allocation.
    //  Differs on IA-32 and AMD64. May tweak virtual memory in the process.

    MainTerminal->Write("[....] Initializing physical memory...");
    Handle res = InitializePhysicalMemory();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize physical memory: %H"
            , res);
    }
}

static __startup void MainInitializeVirtualMemory()
{
    //  Initialize the virtual memory for use by the kernel.
    //  Differs on IA-32 and AMD64.

    MainTerminal->Write("[....] Initializing virtual memory...");
    Handle res = InitializeVirtualMemory();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize virtual memory: %H"
            , res);
    }
}

#ifdef __BEELZEBUB_SETTINGS_UNIT_TESTS
static __startup void MainRunUnitTests()
{
    //  Run all available unit tests, if asked.

    if (CMDO_UnitTests.ParsingResult.IsValid() && CMDO_UnitTests.BooleanValue)
    {
        MainTerminal->Write("[....] Running unit tests... ");

        UnitTestsReport report = RunUnitTests();

        *MainTerminal << report.SuccessCount << "/" << report.TestCount
            << " successful.\r[OKAY]" << EndLine;
    }
}
#endif

static __startup void MainInitializeAcpiTables()
{
    //  Initialize the ACPI tables for easier use.
    //  Mostly common on x86.

    MainTerminal->Write("[....] Initializing ACPI tables...");
    Handle res = InitializeAcpiTables();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize the ACPI tables: %H"
            , res);
    }
}

static __startup void MainInitializeApic()
{
    //  Initialize the LAPIC for the BSP and the I/O APIC.
    //  Mostly common on x86.

    MainTerminal->Write("[....] Initializing APIC...");
    Handle res = InitializeApic();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize the APIC: %H"
            , res);
    }
}

static __startup void MainBootstrapThread()
{
    //  Turns the current system state into a kernel process and a main thread.

    MainTerminal->Write("[....] Initializing as bootstrap thread...");

    Handle res = InitializeBootstrapThread(&BootstrapThread, &BootstrapProcess);

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize main entry point as bootstrap thread: %H"
            , res);
    }

    Cpu::SetThread(&BootstrapThread);
    Cpu::SetProcess(&BootstrapProcess);
}

static __startup void MainInitializeExtraCpus()
{
    //  Initialize the other processing units in the system.
    //  Mostly common on x86, but the executed code differs by arch.

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    //  Note: If SMP is disabled by build, nothing is done, and only the BSP is
    //  used.

    MainTerminal->WriteLine("[SKIP] Kernel was build with SMP disabled. Other processing units ignored.");
#else
    MainShouldElideLocks = false;

    if (CMDO_SmpEnable.ParsingResult.IsValid() && !CMDO_SmpEnable.BooleanValue)
    {
        MainTerminal->WriteLine("[SKIP] Extra processing units ignored as indicated in arguments.");

        CpuDataSetUp = MainShouldElideLocks = true;
        //  Let the kernel know that CPU data is available for use, and elide
        //  useless locks.
    }
    else if (Acpi::PresentLapicCount > 1)
    {
        MainTerminal->Write("[....] Initializing extra processing units...");
        Handle res = InitializeProcessingUnits();

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
        MainTerminal->Write("[TEST] Testing lock elision... ");

        TestLockElision();

        MainTerminal->WriteLine(" Done.");
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
        MainTerminal->Write("[....] Eliding locks...");
        Handle res = ElideLocks();

        if (res.IsOkayResult())
            MainTerminal->WriteLine(" Done.\r[OKAY]");
        else
        {
            MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

            ASSERT(false, "Failed to elide locks: %H", res);
        }
    }
#endif
}

static __startup void MainInitializeBootModules()
{
    //  Initialize the modules loaded by the bootloader with the kernel.
    //  Mostly common.

    MainTerminal->Write("[....] Initializing boot modules...");
    Handle res = InitializeModules();

    if (res.IsOkayResult())
    {
        // if (KernelImage != nullptr)
            MainTerminal->WriteLine(" Done.\r[OKAY]");
        // else
        // {
        //     MainTerminal->WriteFormat(" Fail! No kernel image found.\r[FAIL]%n");

        //     ASSERT(false, "Kernel image hasn't been found!");
        // }
    }
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize modules: %H"
            , res);
    }
}

static __startup void MainInitializeRuntimeLibraries()
{
    //  Initialize the modules loaded with the kernel.
    //  Mostly common.

    MainTerminal->Write("[....] Initializing runtime libraries...");

#if   defined(__BEELZEBUB__ARCH_AMD64)
    Handle res = Runtime64::Initialize();

    if (res.IsOkayResult())
        MainTerminal->Write(" 64-bit...");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize 64-bit runtime: %H"
            , res);
    }
#endif

    MainTerminal->WriteLine(" Done.\r[OKAY]");
    
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
        MainTerminal->Write("[....] Initializing extended thread states...");
        Handle res = ExtendedStates::Initialize(Fpu::StateSize, Fpu::StateAlignment);

        if (res.IsOkayResult())
        {
            MainTerminal->Write(" Allocating template state...");

            void * templateState;

            res = ExtendedStates::AllocateTemplate(templateState);

            if (res.IsOkayResult())
                MainTerminal->WriteLine(" Done.\r[OKAY]");
            else
            {
                MainTerminal->WriteLine("\r[FAIL]%n");
                MainTerminal->WriteLine("       Fail! Could not allocate template state.");

                ASSERT(false, "Failed to allocate template extended thread state: %H"
                    , res);
            }

            Fpu::SaveState(templateState);

            Cpu::SetCr0(Cpu::GetCr0().SetTaskSwitched(true));
        }
        else
        {
            MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

            ASSERT(false, "Failed to initialize extended thread states: %H"
                , res);
        }
    }
    else
    {
        MainTerminal->WriteLine("[SKIP] Extended thread states not needed by present CPU features.");
    }
}

static __startup void MainInitializeSyscalls()
{
    MainTerminal->Write("[....] Initializing syscalls...");
    Syscalls::Initialize();
    MainTerminal->WriteLine(" Done.\r[OKAY]");
}

static __startup void MainInitializeKernelModules()
{
    //  Prepare the kernel for loading modules into itself.
    //  Pretty much architecture-agnostic, at this point at least.

    MainTerminal->Write("[....] Initializing kernel modules...");
    Handle res = Modules::Initialize();

    if (res.IsOkayResult())
        MainTerminal->WriteLine(" Done.\r[OKAY]");
    else
    {
        MainTerminal->WriteFormat(" Fail..? %H\r[FAIL]%n", res);

        ASSERT(false, "Failed to initialize kernel modules: %H"
            , res);
    }
}

static __startup void MainInitializeMainTerminal()
{
    //  Upgrade the terminal to a more capable and useful one.
    //  Yet again, platform-specific.

    MainTerminal->Write("[....] Initializing main terminal...");

    TerminalBase * secondaryTerminal;

    if (CMDO_Term.ParsingResult.IsValid())
        secondaryTerminal = InitializeTerminalMain(CMDO_Term.StringValue);
    else
        secondaryTerminal = InitializeTerminalMain(nullptr);

    MainTerminal->WriteLine(" Done.\r[OKAY]");

    MainTerminal->WriteLine("Switching over.");
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

    new (&Domain0) Domain();
    //  Initialize domain 0. Make sure it's not in a possibly-invalid state.

    InitializationLock.Reset();
    TerminalMessageLock.Reset();
    Domain0.GdtLock.Reset();
    //  Make sure these are clear.

    Domain0.Gdt = GdtRegister::Retrieve();

    withLock (InitializationLock)
    {
        //  First step is getting a simple terminal running for the most
        //  basic of output. This should be x86-common.
        MainTerminal = InitializeTerminalProto();

        MainTerminal->WriteLine("Welcome to Beelzebub!                            (c) 2015 Alexandru-Mihai Maftei");

        Rtc::Read();
        DEBUG_TERM << "Boot time: " << Rtc::Year << '-' << Rtc::Month << '-' << Rtc::Day
        << ' ' << Rtc::Hours << ':' << Rtc::Minutes << ':' << Rtc::Seconds << EndLine;

        MainParseKernelArguments();
        MainInitializeInterrupts();
        MainInitializePhysicalMemory();
        MainInitializeVirtualMemory();

#ifdef __BEELZEBUB_SETTINGS_UNIT_TESTS
        MainRunUnitTests();
#endif

        MainInitializeAcpiTables();
        MainInitializeApic();
        MainInitializeTimers();

        MainBootstrapThread();

        MainInitializeExtraCpus();
        MainElideLocks();

        MainInitializeBootModules();
        MainInitializeRuntimeLibraries();

        MainInitializeFpu();
        MainInitializeSyscalls();
        MainInitializeKernelModules();

        MainInitializeMainTerminal();

        //  Permit other processors to initialize themselves.
        MainTerminal->WriteLine("Initialization complete! Will enable scheduling.");
        MainTerminal->WriteLine();

#if     defined(__BEELZEBUB__TEST_RW_SPINLOCK) && defined(__BEELZEBUB_SETTINGS_SMP)
        if (Cpu::Count.Load() > 1 && CHECK_TEST(RW_SPINLOCK))
        {
            RwSpinlockTestBarrier1.Reset();
            RwSpinlockTestBarrier2.Reset();
            RwSpinlockTestBarrier3.Reset();
        }
#endif

#ifdef __BEELZEBUB__TEST_OBJA
        if (CHECK_TEST(OBJA))
        {
            ObjectAllocatorTestBarrier1.Reset();
            ObjectAllocatorTestBarrier2.Reset();
            ObjectAllocatorTestBarrier3.Reset();
        }
#endif
    }

    Scheduling = true;

    withLock (TerminalMessageLock)
    {
        //  Now every core will print.
        MainTerminal->Write("+-- Core #");
        MainTerminal->WriteUIntD(Cpu::GetData()->Index);
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

#ifdef __BEELZEBUB__TEST_METAP
        if (CHECK_TEST(METAP))
        {
            MainTerminal->Write(">Testing metaprgoramming facilities...");

            TestMetaprogramming();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_EXCP
        if (CHECK_TEST(EXCP))
        {
            MainTerminal->Write(">Testing exceptions...");

            TestExceptions();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_STR
        if (CHECK_TEST(STR))
        {
            MainTerminal->Write(">Testing string.h implementation...");

            TestStringLibrary();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_CMDO
        if (CHECK_TEST(CMDO))
        {
            MainTerminal->Write(">Testing command-line options parsing...");

            TestCmdo();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_TERMINAL
        if (CHECK_TEST(TERMINAL))
        {
            MainTerminal->Write(">Testing terminal implementation(s)...");

            TestTerminal();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_AVL_TREE
        if (CHECK_TEST(AVL_TREE))
        {
            MainTerminal->Write(">Testing AVL trees...");

            TestAvlTree();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_VAS
        if (CHECK_TEST(VAS))
        {
            MainTerminal->Write(">Testing VAS implementation...");

            TestVas();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_KMOD
        if (CHECK_TEST(KMOD))
        {
            MainTerminal->Write(">Testing a kernel module...");

            TestKmod();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_MT
        if (CHECK_TEST(MT))
        {
            MainTerminal->Write(">Starting multitasking test...");

            StartMultitaskingTest();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_STACKINT
        if (CHECK_TEST(STACKINT))
        {
            MainTerminal->Write(">Testing stack integrity...");

            TestStackIntegrity();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_INTERRUPT_LATENCY
        if (CHECK_TEST(INT_LAT))
        {
            MainTerminal->Write(">Testing interrupt latency...");

            TestInterruptLatency();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_BIGINT
        if (CHECK_TEST(BIGINT))
        {
            MainTerminal->Write(">Testing big integer implementation...");

            TestBigInt();

            MainTerminal->WriteLine(" Done.");
        }
#endif

#ifdef __BEELZEBUB__TEST_FPU
        if (CHECK_TEST(FPU))
        {
            MainTerminal->Write(">Testing FPU and SSE...");

            TestFpu();

            MainTerminal->WriteLine(" Done.");
        }
#endif

        MainTerminal->WriteLine("\\Halting indefinitely now.");

        //PrintToDebugTerminal(Domain0.Gdt);
        //msg("%n%n");
    }

#if     defined(__BEELZEBUB__TEST_RW_SPINLOCK) && defined(__BEELZEBUB_SETTINGS_SMP)
    if (Cpu::Count.Load() > 1 && CHECK_TEST(RW_SPINLOCK))
    {
        withLock (TerminalMessageLock)
            MainTerminal->WriteFormat("Core %us: Testing R/W spinlock.%n", Cpu::GetData()->Index);
        
        TestRwSpinlock(true);

        withLock (TerminalMessageLock)
            MainTerminal->WriteFormat("Core %us: Finished R/W spinlock test.%n", Cpu::GetData()->Index);
    }
#endif

#ifdef __BEELZEBUB__TEST_OBJA
    if (CHECK_TEST(OBJA))
    {
        withLock (TerminalMessageLock)
            MainTerminal->WriteFormat("Core %us: Testing fixed-sized object allocator.%n", Cpu::GetData()->Index);
        
        TestObjectAllocator(true);

        withLock (TerminalMessageLock)
            MainTerminal->WriteFormat("Core %us: Finished object allocator test.%n", Cpu::GetData()->Index);
    }
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

    // ApicTimer::Initialize();
    // //  And the APIC timer.

    Syscalls::Initialize();
    //  And syscalls.

    InitializationLock.Spin();
    //  Wait for the system to initialize.

    Fpu::InitializeSecondary();
    //  Meh...

    //  Now every core will print.

    withLock (TerminalMessageLock)
    {
        MainTerminal->Write("+-- Core #");
        MainTerminal->WriteUIntD(Cpu::GetData()->Index);
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
    }

#ifdef __BEELZEBUB__TEST_RW_SPINLOCK
    if (CHECK_TEST(RW_SPINLOCK))
    {
        withLock (TerminalMessageLock)
            MainTerminal->WriteFormat("Core %us: Testing R/W spinlock.%n", Cpu::GetData()->Index);
        
        TestRwSpinlock(false);

        withLock (TerminalMessageLock)
            MainTerminal->WriteFormat("Core %us: Finished R/W spinlock test.%n", Cpu::GetData()->Index);
    }
#endif

#ifdef __BEELZEBUB__TEST_OBJA
    if (CHECK_TEST(OBJA))
    {
        withLock (TerminalMessageLock)
            MainTerminal->WriteFormat("Core %us: Testing fixed-sized object allocator.%n", Cpu::GetData()->Index);

        TestObjectAllocator(false);

        withLock (TerminalMessageLock)
            MainTerminal->WriteFormat("Core %us: Finished object allocator test.%n", Cpu::GetData()->Index);
    }
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
    // COM1 = ManagedSerialPort(0x3F8);
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

    //  And returns it.
    //return &initialSerialTerminal; // termPtr;
    return &initialVbeTerminal;
}

TerminalBase * InitializeTerminalMain(char * clue)
{
    if (clue == nullptr || strcmp("vbe", clue) == 0)
        return &initialVbeTerminal;

    if (strcmp("serial", clue) == 0)
        return &initialSerialTerminal;

    assert_or(false, "Unknown terminal type: %s", clue)
    {
        return &initialVbeTerminal;
    }
}

/*****************
    INTERRUPTS
*****************/

Handle InitializeInterrupts()
{
    size_t isrStubsSize = (size_t)(&IsrStubsEnd - &IsrStubsBegin);

    assert(isrStubsSize == Interrupts::Count
        , "ISR stubs seem to have the wrong size!");
    //  The ISR stubs must be aligned to avoid a horribly repetition.

    for (size_t i = 0; i < Interrupts::Count; ++i)
    {
        Interrupts::Get(i)
        .SetGate(
            IdtGate()
            .SetOffset(&IsrStubsBegin + i)
            .SetSegment(Gdt::KernelCodeSegment)
            .SetType(IdtGateType::InterruptGate)
            .SetPresent(true))
        .SetHandler(&MiscellaneousInterruptHandler)
        .SetEnder(nullptr);
    }

    //  So now the IDT should be fresh out of the oven and ready for serving.

#if   defined(__BEELZEBUB__ARCH_AMD64)
    Interrupts::Get(KnownExceptionVectors::DoubleFault).GetGate()->SetIst(1);
    Interrupts::Get(KnownExceptionVectors::PageFault  ).GetGate()->SetIst(2);
#endif

    Interrupts::Get(KnownExceptionVectors::DivideError).SetHandler(&DivideErrorHandler);
    Interrupts::Get(KnownExceptionVectors::Breakpoint).SetHandler(&BreakpointHandler);
    Interrupts::Get(KnownExceptionVectors::Overflow).SetHandler(&OverflowHandler);
    Interrupts::Get(KnownExceptionVectors::BoundRangeExceeded).SetHandler(&BoundRangeExceededHandler);
    Interrupts::Get(KnownExceptionVectors::InvalidOpcode).SetHandler(&InvalidOpcodeHandler);
    Interrupts::Get(KnownExceptionVectors::NoMathCoprocessor).SetHandler(&NoMathCoprocessorHandler);
    Interrupts::Get(KnownExceptionVectors::DoubleFault).SetHandler(&DoubleFaultHandler);
    Interrupts::Get(KnownExceptionVectors::InvalidTss).SetHandler(&InvalidTssHandler);
    Interrupts::Get(KnownExceptionVectors::SegmentNotPresent).SetHandler(&SegmentNotPresentHandler);
    Interrupts::Get(KnownExceptionVectors::StackSegmentFault).SetHandler(&StackSegmentFaultHandler);
    Interrupts::Get(KnownExceptionVectors::GeneralProtectionFault).SetHandler(&GeneralProtectionHandler);
    Interrupts::Get(KnownExceptionVectors::PageFault).SetHandler(&PageFaultHandler);

    Interrupts::Get(KnownExceptionVectors::ApicTimer).RemoveHandler();

    Pic::Initialize(0x20);  //  Just below the spurious interrupt vector.

    Pic::Subscribe(1, &keyboard_handler);
    Pic::Subscribe(3, &SerialPort::IrqHandler);
    Pic::Subscribe(4, &SerialPort::IrqHandler);

    Interrupts::Register.Activate();

    return HandleResult::Okay;
}

/**********
    PIT
**********/

Handle InitializeTimers()
{
    PitCommand pitCmd {};
    pitCmd.SetAccessMode(PitAccessMode::LowHigh);
    pitCmd.SetOperatingMode(PitOperatingMode::SquareWaveGenerator);

    Pit::SendCommand(pitCmd);

    ApicTimer::Initialize();

    MainTerminal->WriteFormat(" APIC @ %u8 Hz...", ApicTimer::Frequency);

    Pit::SetHandler();

    Pit::SetFrequency(1000);
    //  This frequency really shouldn't stress the BSP that much, considering
    //  that the IRQ would get 2-3 million clock cycles on modern chips.

    MainTerminal->WriteFormat(" PIT @ %u4 Hz...", Pit::Frequency);

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

    res = Acpi::FindRsdp(VmmArc::IsaDmaStart + 0x0E0000
                       , VmmArc::IsaDmaStart + 0x100000);

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

    res = Vmm::MapPage(&BootstrapProcess, Lapic::VirtualAddress, lapicPaddr
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryMapOptions::NoReferenceCounting);

    ASSERT(res.IsOkayResult()
        , "Failed to map page at %Xp (%XP) for LAPIC: %H%n"
        , Lapic::VirtualAddress, lapicPaddr, res);

    res = Lapic::Initialize();
    //  This initializes the LAPIC for the BSP.

    ASSERT(res.IsOkayResult()
        , "Failed to initialize the LAPIC?! %H%n"
        , res);

    if (Cpu::GetData()->X2ApicMode)
        MainTerminal->Write(" Local x2APIC...");
    else
        MainTerminal->Write(" LAPIC...");

    if (Acpi::IoapicCount < 1)
    {
        MainTerminal->Write(" no I/O APIC...");

        return HandleResult::Okay;
    }
    
    // uintptr_t madtEnd = (uintptr_t)Acpi::MadtPointer + Acpi::MadtPointer->Header.Length;

    // uintptr_t e = (uintptr_t)Acpi::MadtPointer + sizeof(*Acpi::MadtPointer);
    // for (/* nothing */; e < madtEnd; e += ((acpi_subtable_header *)e)->Length)
    // {
    //     if (ACPI_MADT_TYPE_IO_APIC != ((acpi_subtable_header *)e)->Type)
    //         continue;

    //     auto ioapic = (acpi_madt_io_apic *)e;

    //     MainTerminal->WriteFormat("%n%*(( MADTe: I/O APIC ID-%u1 ADDR-%X4 GIB-%X4 ))"
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

    //             MainTerminal->WriteFormat("%n%*(( MADTe: INT OVR BUS-%u1 SIRQ-%u1 GIRQ-%X4 IFLG-%X2 ))"
    //                 , (size_t)23, intovr->Bus, intovr->SourceIrq, intovr->GlobalIrq, intovr->IntiFlags);
    //         }
    //         break;

    //     case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
    //         {
    //             auto lanmi = (acpi_madt_local_apic_nmi *)e;

    //             MainTerminal->WriteFormat("%n%*(( MADTe: LA NMI PID-%u1 IFLG-%X2 LINT-%u1 ))"
    //                 , (size_t)32, lanmi->ProcessorId, lanmi->IntiFlags, lanmi->Lint);
    //         }
    //         break;
    //     }
    // }

    return HandleResult::Okay;
}

/***********************
    PROCESSING UNITS
***********************/

__startup Handle InitializeAp(uint32_t const lapicId
                            , uint32_t const procId
                            ,   size_t const apIndex);

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

    res = Vmm::MapPage(&BootstrapProcess, bootstrapVaddr, bootstrapPaddr
        , MemoryFlags::Global | MemoryFlags::Executable | MemoryFlags::Writable
        , MemoryMapOptions::NoReferenceCounting);

    ASSERT(res.IsOkayResult()
        , "Failed to map page at %Xp (%XP) for init code: %H%n"
        , bootstrapVaddr, bootstrapPaddr, res);

    res = Vmm::InvalidatePage(nullptr, bootstrapVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to invalidate page at %Xp (%XP) for init code: %H%n"
        , bootstrapVaddr, bootstrapPaddr, res);

    BootstrapPml4Address = BootstrapProcess.PagingTable;

    COMPILER_MEMORY_BARRIER();
    //  Needed to make sure that the PML4 address is copied over.

    memcpy((void *)bootstrapVaddr, &ApBootstrapBegin, (uintptr_t)&ApBootstrapEnd - (uintptr_t)&ApBootstrapBegin);
    //  This makes sure the code can be executed by the AP.

    KernelGdtPointer = GdtRegister::Retrieve();

    BREAKPOINT_SET_AUX((int volatile *)((uintptr_t)&ApBreakpointCookie - (uintptr_t)&ApBootstrapBegin + bootstrapVaddr));
    int_cookie_t const int_cookie = Interrupts::PushEnable();

    // MainTerminal->WriteFormat("%n      PML4 addr: %XP, GDT addr: %Xp; BSP LAPIC ID: %u4"
    //     , BootstrapPml4Address, KernelGdtPointer.Pointer, Lapic::GetId());

    uintptr_t madtEnd = (uintptr_t)Acpi::MadtPointer + Acpi::MadtPointer->Header.Length;
    uintptr_t e = (uintptr_t)Acpi::MadtPointer + sizeof(*Acpi::MadtPointer);
    for (/* nothing */; e < madtEnd; e += ((acpi_subtable_header *)e)->Length)
    {
        if (ACPI_MADT_TYPE_LOCAL_APIC != ((acpi_subtable_header *)e)->Type)
            continue;

        auto lapic = (acpi_madt_local_apic *)e;

        // MainTerminal->WriteFormat("%n%*(( MADTe: %s LAPIC LID-%u1 PID-%u1 F-%X4 ))"
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

    Interrupts::RestoreState(int_cookie);
    BREAKPOINT_SET_AUX(nullptr);

    res = Vmm::UnmapPage(&BootstrapProcess, bootstrapVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to unmap unneeded page at %Xp (%XP) for init code: %H%n"
        , bootstrapVaddr, bootstrapPaddr, res);

    CpuDataSetUp = true;
    //  Let the kernel know that CPU data is available for use.

    return HandleResult::Okay;
}

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

Handle InitializeAp(uint32_t const lapicId
                  , uint32_t const procId
                  , size_t const apIndex)
{
    Handle res;
    //  Intermediate results.

    vaddr_t vaddr = nullvaddr;

    res = Vmm::AllocatePages(nullptr
        , CpuStackSize / PageSize
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

    ApStackTopPointer = vaddr + CpuStackSize;
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
