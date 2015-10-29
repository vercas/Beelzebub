#include <synchronization/atomic.hpp>
#include <memory/manager_amd64.hpp>
#include <memory/virtual_allocator.hpp>
#include <memory/paging.hpp>
#include <memory/page_allocator.hpp>
#include <system/cpu.hpp>
#include <system/cpuid.hpp>
#include <system/lapic.hpp>
#include <system/exceptions.hpp>
#include <system/interrupts.hpp>
#include <system/isr.hpp>
#include <terminals/serial.hpp>
#include <terminals/vbe.hpp>

#include <jegudiel.h>
#include <multiboot.h>
#include <keyboard.hpp>

#include <kernel.hpp>
#include <debug.hpp>
#include <architecture.h>
#include <entry.h>
#include <math.h>
#include <string.h>

#include <_print/registers.hpp>
#include <_print/gdt.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Ports;
using namespace Beelzebub::System;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Terminals;

/*  Constants  */

static size_t const PageSize = PAGE_SIZE;

//uint8_t initialVbeTerminal[sizeof(SerialTerminal)];
SerialTerminal initialSerialTerminal;
VbeTerminal initialVbeTerminal;

PageAllocationSpace mainAllocationSpace;
PageAllocator mainAllocator;
VirtualAllocationSpace bootstrapVas;
MemoryManagerAmd64 bootstrapMemMgr;

Atomic<bool> bootstrapReady {false};

CpuId Beelzebub::System::BootstrapProcessorId;

/*  CPU data  */

Atomic<uintptr_t> CpuDataBase;
Atomic<size_t> CpuIndexCounter {(size_t)0};

__bland void InitializeCpuData()
{
    uintptr_t const loc = CpuDataBase.FetchAdd(Cpu::CpuDataSize);
    CpuData * data = (CpuData *)loc;

    Msrs::Write(Msr::IA32_GS_BASE, (uint64_t)loc);

    size_t const ind = CpuIndexCounter++;

    data->Index = ind;

    data->HeapSpinlock = Spinlock<>().GetValue();
    data->HeapSpinlockPointer = (Spinlock<> *)&data->HeapSpinlock;

    assert(Cpu::GetIndex() == ind
        , "Failed to set CPU index..? It should be %us but %us is returned."
        , ind, Cpu::GetIndex());

    //msg("-- Core #%us @ %Xp. --%n", ind, gs_base);
}

/*  Entry points  */ 

void kmain_bsp()
{
    bootstrapReady = false;
    //  Just makin' sure.

#if   !defined(__BEELZEBUB_SETTINGS_NO_SMP)
    Cpu::Count = 1;
#endif

    IsrHandlers[KEYBOARD_IRQ_VECTOR] = &keyboard_handler;
    keyboard_init();
    IsrHandlers[(uint8_t)KnownExceptionVectors::PageFault] = &PageFaultHandler;

    //breakpoint();

    Beelzebub::Main();
}

void kmain_ap()
{
    while (!bootstrapReady) ;
    //  Await!

#if   !defined(__BEELZEBUB_SETTINGS_NO_SMP)
    ++Cpu::Count;
#endif

    if (VirtualAllocationSpace::NX)
        Cpu::EnableNxBit();

    bootstrapVas.Activate();
    //bootstrapMemMgr.Activate();
    //  Perfectly valid solution.

    InitializeCpuData();

    //while (true) { }

    Beelzebub::Secondary();
}

/*  Terminal  */

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

/*  Interrupts  */

Handle InitializeInterrupts()
{
    Lapic::Initialize();

    for (size_t i = 0; i < 256; ++i)
    {
        IsrHandlers[i] = &MiscellaneousInterruptHandler;
    }

    IsrHandlers[(uint8_t)KnownExceptionVectors::DivideError] = &DivideErrorHandler;
    IsrHandlers[(uint8_t)KnownExceptionVectors::Overflow] = &OverflowHandler;
    IsrHandlers[(uint8_t)KnownExceptionVectors::BoundRangeExceeded] = &BoundRangeExceededHandler;
    IsrHandlers[(uint8_t)KnownExceptionVectors::InvalidOpcode] = &InvalidOpcodeHandler;
    IsrHandlers[(uint8_t)KnownExceptionVectors::DoubleFault] = &DoubleFaultHandler;
    IsrHandlers[(uint8_t)KnownExceptionVectors::InvalidTss] = &InvalidTssHandler;
    IsrHandlers[(uint8_t)KnownExceptionVectors::SegmentNotPresent] = &SegmentNotPresentHandler;
    IsrHandlers[(uint8_t)KnownExceptionVectors::StackSegmentFault] = &StackSegmentFaultHandler;
    IsrHandlers[(uint8_t)KnownExceptionVectors::GeneralProtectionFault] = &GeneralProtectionHandler;
    IsrHandlers[(uint8_t)KnownExceptionVectors::PageFault] = &PageFaultHandler;

    IsrHandlers[32] = &SerialPort::IrqHandler;

    IsrHandlers[KEYBOARD_IRQ_VECTOR] = &keyboard_handler;

    //initialVbeTerminal.WriteHex64((uint64_t)&SerialPort::IrqHandler);

    return HandleResult::Okay;
}

/**********************************************/
/*  Memory map sanitation and initialization  */
/**********************************************/

bool firstRegionCreated;

psize_t currentSpaceIndex, currentSpaceLimit;
PageAllocationSpace * currentSpaceLocation;

__bland PageAllocationSpace * CreateAllocationSpace(paddr_t start, paddr_t end)
{
    /*msg("ALLOC SPACE: %XP-%XP; ", start, end);//*/

    if (firstRegionCreated)
    {
        if (currentSpaceIndex == currentSpaceLimit)
        {
            //  The limit of the current allocation space location has
            //  been reached.

            currentSpaceIndex = 0;
            //  Reset the index.

            currentSpaceLocation = nullptr;
            //  Nullify the location so it is recreated.

            /*msg("MAX; ");//*/
        }

        if (currentSpaceLocation == nullptr)
        {
            PageDescriptor * desc = nullptr;

            currentSpaceLocation = (PageAllocationSpace *)mainAllocator.AllocatePage(desc);

            assert(currentSpaceLocation != nullptr && desc != nullptr
                , "Unable to allocate a special page for creating more allocation spaces!");

            Handle res = mainAllocator.ReserveByteRange((paddr_t)currentSpaceLocation, PageSize, PageReservationOptions::IncludeInUse);

            assert(res.IsOkayResult()
                , "Failed to reserve special page for further allocation space creation: %H"
                , res);

            /*msg("PAGE@%Xp; ", currentSpaceLocation);//*/
        }

        /*msg("SPA@%Xp; I=%u2; "
            , currentSpaceLocation + currentSpaceIndex
            , (uint16_t)currentSpaceIndex);//*/

        PageAllocationSpace * space = new (currentSpaceLocation + currentSpaceIndex++) PageAllocationSpace(start, end, PageSize);
        //  One of the really neat things I like about C++.

        space->InitializeControlStructures();

        mainAllocator.PreppendAllocationSpace(space);

        /*msg("%XP-%XP;%n"
            , space->GetAllocationStart()
            , space->GetAllocationEnd());//*/

        return space;
    }
    else
    {
        new (&mainAllocationSpace) PageAllocationSpace(start, end, PageSize);

        mainAllocationSpace.InitializeControlStructures();

        new (&mainAllocator)       PageAllocator(&mainAllocationSpace);

        firstRegionCreated = true;

        currentSpaceIndex = 0;
        currentSpaceLimit = PageSize / sizeof(PageAllocationSpace);
        currentSpaceLocation = nullptr;

        /*msg("FIRST; SPA@%Xp; ALC@%Xp; %XP-%XP; M=%u2,S=%u2%n"
            , &mainAllocationSpace, &mainAllocator
            , mainAllocationSpace.GetAllocationStart()
            , mainAllocationSpace.GetAllocationEnd()
            , (uint16_t)currentSpaceLimit
            , (uint16_t)sizeof(PageAllocationSpace));//*/

        MainPageAllocator = &mainAllocator;

        return &mainAllocationSpace;
    }
}

__bland void RemapTerminal(TerminalBase * const terminal, VirtualAllocationSpace * const space)
{
    Handle res;

    if (terminal->Descriptor->Capabilities.Type == TerminalType::PixelMatrix)
    {
        VbeTerminal * const term = (VbeTerminal *)terminal;
        const size_t size = ((size_t)term->Pitch * (size_t)term->Height + PageSize - 1) & ~0xFFFULL;
        //  Yes, the size is aligned with page boundaries.

        if (term->VideoMemory >= MemoryManagerAmd64::KernelModulesStart
         && term->VideoMemory <  MemoryManagerAmd64::KernelModulesEnd)
            return;
        //  Nothing to do, folks.
        
        uintptr_t const loc = MemoryManagerAmd64::KernelHeapCursor.FetchAdd(size);

        size_t off = 0;

        do
        {
            res = space->Map(loc + off, term->VideoMemory + off, PageFlags::Global | PageFlags::Writable);

            assert(res.IsOkayResult()
                , "Failed to map page for VBE terminal (%Xp to %Xp): %H"
                , loc + off, term->VideoMemory + off
                , res);

            off += PageSize;
        } while (off < size);

        term->VideoMemory = loc;
    }

    //  TODO: Make a VGA text terminal and also handle it here.
}

__cold __bland void SanitizeAndInitializeMemory(jg_info_mmap_t * map, uint32_t cnt, uintptr_t freeStart)
{
    Handle res; //  Used for intermediary results.

    //  First step is aligning the memory map.
    //  Also, yes, I could've used bits 'n powers of two here.
    //  But I'm hoping to future-proof the code a bit, in case
    //  I would ever target a platform whose page size is not
    //  A power of two.
    //  Moreover, this code doesn't need to be lightning-fast. :L

    uint64_t start = RoundUp(freeStart, PageSize),
               end = 0;
    jg_info_mmap_t * firstMap = nullptr,
                   *  lastMap = nullptr;

    for (uint32_t i = 0; i < cnt; i++)
    {
        jg_info_mmap_t * m = map + i;
        //  Current map.

        if ((m->address + m->length) <= freeStart || !m->available)
            continue;

        if (firstMap == nullptr)
            firstMap = m;

        /*
        uint64_t addressMisalignment = RoundUpDiff(m->address, PageSize);
        //  The address is rounded up to the closest page;

        m->address += addressMisalignment;
        m->length -= addressMisalignment;

        m->length -= m->length % PageSize;
        //  The length is rounded down.
        */// This is not necessary when using Jegudiel.

        uint64_t mEnd = m->address + m->length;

        if (mEnd > end)
        {
            end = mEnd;
            lastMap = m;
        }
    }

    //  DUMPING MMAP

    /*initialVbeTerminal.WriteLine("IND |A|    Address     |     Length     |       End      |");

    for (uint32_t i = 0; i < cnt; i++)
    {
        jg_info_mmap_t & e = map[i];
        //  Current map.

        //if (e.available == 0) continue;

        initialVbeTerminal.WriteHex16((uint16_t)i);
        initialVbeTerminal.Write("|");
        initialVbeTerminal.Write(e.available ? "A" : " ");
        initialVbeTerminal.Write("|");
        initialVbeTerminal.WriteHex64(e.address);
        initialVbeTerminal.Write("|");
        initialVbeTerminal.WriteHex64(e.length);
        initialVbeTerminal.Write("|");
        initialVbeTerminal.WriteHex64(e.address + e.length);
        initialVbeTerminal.WriteLine("|");
    }

    initialVbeTerminal.Write("Free memory start: ");
    initialVbeTerminal.WriteHex64(freeStart);

    msg("%n");

    initialVbeTerminal.WriteFormat("Initializing memory over entries #%us-%us...%n", (size_t)(firstMap - map)
                                                        , (size_t)(lastMap - map));
    initialVbeTerminal.WriteFormat(" Address rage: %X8-%X8.%n", start, end);
    initialVbeTerminal.WriteFormat(" Maps start at %Xp.%n", firstMap);

    msg("%n");//*/

    /*Cr3 cr3 = Cpu::GetCr3();
    Pml4 & pml4 = *cr3.GetPml4Ptr();

    pml4.PrintToTerminal(&initialSerialTerminal);

    for (uint16_t i = 0; i < 1; ++i)
    {
        Pml4Entry & e = pml4[i];
        Pml3 & pml3 = *e.GetPml3Ptr();

        pml3.PrintToTerminal(&initialSerialTerminal);
    }*/

    //  SPACE CREATION

    for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
        if (m->available != 0 && m->length >= (2 * PageSize))
        {
            if (m->address < start && (m->address + m->length) > start)
                //  Means this entry crosses the start of free memory.
                CreateAllocationSpace(start, m->address + m->length);
            else
                CreateAllocationSpace(m->address, m->address + m->length);
        }

    //  PAGE RESERVATION

#ifdef __BEELZEBUB__DEBUG
    //mainAllocationSpace.PrintStackToTerminal(&initialVbeTerminal, true);
#endif

    for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
        if (m->available == 0)
        {
            res = mainAllocator.ReserveByteRange(m->address, m->length);

            assert(res.IsOkayResult() || res.IsResult(HandleResult::PagesOutOfAllocatorRange)
                , "Failed to reserve page range %XP-%XP: %H."
                , m->address, m->address + m->length
                , res);
        }

#ifdef __BEELZEBUB__DEBUG
    //mainAllocationSpace.PrintStackToTerminal(&initialVbeTerminal, true);
#endif

    //  PAGING INITIALIZATION

    //msg("Constructing bootstrap virtual allocation space... ");
    new (&bootstrapVas) VirtualAllocationSpace(&mainAllocator);
    //msg("Done.%n");

    //msg("Bootstrapping the VAS... ");
    bootstrapVas.Bootstrap();
    //msg("Done.%n");

    RemapTerminal(MainTerminal, &bootstrapVas);
    RemapTerminal(Debug::DebugTerminal, &bootstrapVas);

    //initialVbeTerminal.WriteFormat("Statically initializing the memory manager... ");
    MemoryManager::Initialize();
    //initialVbeTerminal.WriteFormat("Done.%n");

    //initialVbeTerminal.WriteFormat("Constructing bootstrap memory manager... ");
    new (&bootstrapMemMgr) MemoryManagerAmd64(&bootstrapVas);
    //initialVbeTerminal.WriteFormat("Done.%n");

    BootstrapMemoryManager = &bootstrapMemMgr;

    //  CPU DATA

    size_t const cpuDataSize = JG_INFO_ROOT_EX->cpu_count * Cpu::CpuDataSize;
    size_t const cpuDataPageCount = (cpuDataSize + PageSize - 1) / PageSize;

    //  This is where the CPU data will sit.
    CpuDataBase.Store(MemoryManagerAmd64::KernelModulesCursor.Load());
    //  Ain't atomic and it doesn't need to be right now.

    MemoryManagerAmd64::KernelModulesCursor += cpuDataPageCount << 12;
    //  The CPU data will snuggle in with the kernel modules. Whoopsie!
    //  And advance the cursor...

    for (size_t i = 0; i < cpuDataPageCount; ++i)
    {
		PageDescriptor * desc = nullptr;

		vaddr_t const vaddr = CpuDataBase + (i << 12);
		paddr_t const paddr = mainAllocator.AllocatePage(desc);

		assert(paddr != nullpaddr && desc != nullptr
			, "  Unable to allocate a physical page for CPU-specific data!");

		res = BootstrapMemoryManager->MapPage(vaddr, paddr, PageFlags::Global | PageFlags::Writable);

		assert(res.IsOkayResult()
			, "  Failed to map page at %Xp (%XP) for CPU-specific data: %H."
			, vaddr, paddr
			, res);

        //initialVbeTerminal.WriteFormat("  Allocated page for CPU data: %Xp -> %XP.%n", vaddr, paddr);
    }

    InitializeCpuData();

    //  TODO: Parse ACPI, get LAPIC paddr.

    //  RELEASING APs

    bootstrapReady = true;

    //msg("%n");

    //Beelzebub::Memory::Initialize(&mainAllocationSpace, 1);
}

__cold __bland Handle HandleKernelModule(const size_t index, const jg_info_module_t * const module, const size_t size)
{
    msg("THIS IS THE KERNEL MODULE!%n");

    return HandleResult::Okay;
}

__cold __bland Handle HandleModule(const size_t index, const jg_info_module_t * const module)
{
    Handle res = HandleResult::Okay;

    size_t const size = (module->length + PageSize - 1) / PageSize;

    msg("Module #%us:%n"
        "\tName: (%X2) %s%n"
        "\tCommand-line: (%X2) %s%n"
        "\tPhysical Address: %XP%n"
        "\tSize: %X4 (%Xs)%n"
        , index
        , module->name, JG_INFO_STRING_EX + module->name
        , module->cmdline, JG_INFO_STRING_EX + module->cmdline
        , module->address
        , module->length, size);//*/

    vaddr_t const vaddr = MemoryManagerAmd64::KernelModulesCursor.FetchAdd(size);

    for (vaddr_t offset = 0; offset < size; offset += PageSize)
    {
        res = BootstrapMemoryManager->MapPage(vaddr + offset, module->address + offset, PageFlags::Global | PageFlags::Writable);

        assert(res.IsOkayResult()
            , "  Failed to map page at %Xp (%XP) for module #%us (%s): %H."
            , vaddr + offset, module->address + offset
            , index, module->name
            , res);
    }

    if (memeq("kernel64", JG_INFO_STRING_EX + module->name, 9))
        return HandleKernelModule(index, module, size);

    return res;
}

Handle InitializeMemory()
{
    BootstrapProcessorId = CpuId();
    BootstrapProcessorId.Initialize();
    //  This is required to page all the available memory.

    //breakpoint();

    BootstrapProcessorId.PrintToTerminal(DebugTerminal);

    //breakpoint();

    msg("%n");

    //  TODO: Take care of the 1-MiB to 16-MiB region for ISA DMA.

    SanitizeAndInitializeMemory(JG_INFO_MMAP_EX, JG_INFO_ROOT_EX->mmap_count, JG_INFO_ROOT_EX->free_paddr);

    msg("%n");

    //  DUMPING CONTROL REGISTERS

    PrintToDebugTerminal(Cpu::GetCr0());
    msg("%n");

    msg("CR2: %Xs%n", (uint64_t)Cpu::GetCr2());

    PrintToDebugTerminal(Cpu::GetCr3());
    msg("%n");

    msg("CR2: %Xs%n", (uint64_t)Cpu::GetCr4());

    msg("%n");

    CpuInstructions::WriteBackAndInvalidateCache();

    auto GDTR = GdtRegister::Retrieve();

    PrintToDebugTerminal(GDTR);

    msg("%n%n");

    return HandleResult::Okay;
}

/*  Modules  */

Handle InitializeModules()
{
    Handle res;

    size_t const moduleCount = (size_t)JG_INFO_ROOT_EX->module_count;

    for (size_t i = 0; i < moduleCount; ++i)
    {
        res = HandleModule(i, JG_INFO_MODULE_EX + i);

        //  TODO: Perhaps handle some (non-)fatal errors here..?
    }

    return res;
}

#ifdef __BEELZEBUB__TEST_MT

Thread tTa1;
Thread tTa2;
Thread tTa3;

Thread tTb1;
Thread tTb2;
Thread tTb3;

Process tPb;
MemoryManagerAmd64 tMmB;
VirtualAllocationSpace tVasB;

__hot __bland void * TestThreadEntryPoint(void * const arg)
{
   char * const myChars = (char *)(uintptr_t)0x321000ULL;

    while (true)
    {
        Thread * activeThread = Cpu::GetActiveThread();

        msg("Printing from thread %Xp! (%C)%n", activeThread, myChars);

        CpuInstructions::Halt();
    }
}

__cold __bland void InitializeTestThread(Thread * const t, Process * const p)
{
    new (t) Thread(p);

    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    vaddr_t const vaddr = MemoryManagerAmd64::KernelHeapCursor.FetchAdd(0x1000);
    paddr_t const paddr = mainAllocator.AllocatePage(desc);
    //  Stack page.

    assert(paddr != nullpaddr && desc != nullptr
        , "  Unable to allocate a physical page for test thread %Xp (process %Xp)!"
        , t, p);

    desc->IncrementReferenceCount();
    //  Increment the reference count of the page because we're nice people.

    res = BootstrapMemoryManager->MapPage(vaddr, paddr, PageFlags::Global | PageFlags::Writable);

    assert(res.IsOkayResult()
        , "  Failed to map page at %Xp (%XP) for test thread stack: %H."
        , vaddr, paddr
        , res);
    //  FAILURE IS NOT TOLERATED.

    t->KernelStackTop = (uintptr_t)vaddr + 0x1000;
    t->KernelStackBottom = (uintptr_t)vaddr;

    t->EntryPoint = &TestThreadEntryPoint;

    InitializeThreadState(t);
    //  This sets up the thread so it goes directly to the entry point when switched to.
}

__cold __bland char * AllocateTestPage(Process * const p)
{
    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    vaddr_t const vaddr1 = 0x321000;
    vaddr_t const vaddr2 = MemoryManagerAmd64::KernelHeapCursor.FetchAdd(0x1000);
    paddr_t const paddr = mainAllocator.AllocatePage(desc);
    //  Test page.

    assert(paddr != nullpaddr && desc != nullptr
        , "  Unable to allocate a physical page for test page of process %Xp!"
        , p);

    desc->IncrementReferenceCount();
    desc->IncrementReferenceCount();
    //  Increment the reference count of the page twice because we're nice people.

    res = p->Memory->MapPage(vaddr1, paddr, PageFlags::Writable);

    assert(res.IsOkayResult()
        , "  Failed to map page at %Xp (%XP) as test page in owning process: %H."
        , vaddr1, paddr
        , res);

    res = BootstrapMemoryManager->MapPage(vaddr2, paddr, PageFlags::Global | PageFlags::Writable);

    assert(res.IsOkayResult()
        , "  Failed to map page at %Xp (%XP) as test page with boostrap memory manager: %H."
        , vaddr2, paddr
        , res);

    return (char *)(uintptr_t)vaddr2;
}

void StartMultitaskingTest()
{
    bootstrapVas.Clone(&tVasB);
    //  Fork the VAS.

    new (&tMmB) MemoryManagerAmd64(&tVasB);
    //  Initialize a new memory manager with the given VAS.

    new (&tPb) Process(&tMmB);
    //  Initialize a new process for thread series B.

    char * tCa = AllocateTestPage(&BootstrapProcess);
    //  Allocate and map a page for process B.

    char * tCb = AllocateTestPage(&tPb);
    //  Allocate and map a page for process B.

    *tCa = 'A';
    *tCb = 'B';
    //  Assign different values.

    InitializeTestThread(&tTa1, &BootstrapProcess);
    InitializeTestThread(&tTa2, &BootstrapProcess);
    InitializeTestThread(&tTa3, &BootstrapProcess);
    //  Initialize thread series A under the bootstrap process.

    InitializeTestThread(&tTb1, &tPb);
    InitializeTestThread(&tTb2, &tPb);
    InitializeTestThread(&tTb3, &tPb);
    //  Initialize thread series B under their own process.

    BootstrapThread.IntroduceNext(&tTb3);
    BootstrapThread.IntroduceNext(&tTb2);
    //  Threads B2 and B3 are at the end of the cycle.

    BootstrapThread.IntroduceNext(&tTa3);
    BootstrapThread.IntroduceNext(&tTb1);
    //  Threads B1 and A3 are intertwined.

    BootstrapThread.IntroduceNext(&tTa2);
    BootstrapThread.IntroduceNext(&tTa1);
    //  Threads A1 and A2 are at the start, right after the bootstrap thread.
}

#endif
