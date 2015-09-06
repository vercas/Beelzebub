#include <architecture.h>
#include <entry.h>
#include <memory/manager_amd64.hpp>
#include <memory/virtual_allocator.hpp>
#include <memory/paging.hpp>
#include <system/cpu.hpp>
#include <system/cpuid.hpp>

#include <jegudiel.h>
#include <multiboot.h>
#include <isr.h>
#include <keyboard.h>
#include <screen.h>
#include <ui.h>

#include <lapic.h>
#include <screen.h>

#include <terminals/serial.hpp>
#include <terminals/vbe.hpp>
#include <memory/page_allocator.hpp>
#include <system/exceptions.hpp>
#include <kernel.hpp>
#include <debug.hpp>
#include <math.h>
#include <cpp_support.h>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Ports;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Memory::Paging;

/*  Constants  */

static const size_t PageSize = PAGE_SIZE;

//uint8_t initialVbeTerminal[sizeof(SerialTerminal)];
SerialTerminal initialSerialTerminal;
VbeTerminal initialVbeTerminal;

PageAllocationSpace mainAllocationSpace;
PageAllocator mainAllocator;
VirtualAllocationSpace bootstrapVas;
MemoryManagerAmd64 bootstrapMemMgr;

volatile bool bootstrapReady = false;

CpuId Beelzebub::System::BootstrapProcessorId;

/*  CPU data  */

volatile uintptr_t CpuDataBase;
volatile size_t CpuIndexCounter = (size_t)0;

__bland void InitializeCpuData()
{
    uintptr_t loc = __atomic_fetch_add(&CpuDataBase, Cpu::CpuDataSize, __ATOMIC_SEQ_CST);
    CpuData * data = (CpuData *)loc;

    Cpu::WriteMsr(Msr::IA32_GS_BASE, (uint64_t)loc);

    size_t ind = __atomic_fetch_add(&CpuIndexCounter, 1, __ATOMIC_SEQ_CST);

    data->Index = ind;

    data->HeapSpinlock = Spinlock().GetValue();
    data->HeapSpinlockPointer = (Spinlock *)&data->HeapSpinlock;

    assert(Cpu::GetIndex() == ind
        , "Failed to set CPU index..? It should be %us but it %us is returned."
        , ind, Cpu::GetIndex());

    //msg("-- Core #%us @ %Xp.%n", ind, loc);
}

/*  Entry points  */ 

void kmain_bsp()
{
    bootstrapReady = false;

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

    Beelzebub::Debug::DebugTerminal = &initialVbeTerminal;

    msg("VM: %Xp; W: %u2, H: %u2, P: %u2; BPP: %u1.%n", (uintptr_t)mbi->framebuffer_addr, (uint16_t)mbi->framebuffer_width, (uint16_t)mbi->framebuffer_height, (uint16_t)mbi->framebuffer_pitch, (uint8_t)mbi->framebuffer_bpp);

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

void InitializeInterrupts()
{
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
        
        const uintptr_t loc = __atomic_fetch_add(&MemoryManagerAmd64::KernelModulesCursor, size, __ATOMIC_SEQ_CST);

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

void SanitizeAndInitializeMemory(jg_info_mmap_t * map, uint32_t cnt, uintptr_t freeStart)
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

    initialVbeTerminal.WriteLine("IND |A|    Address     |     Length     |       End      |");

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

    initialVbeTerminal.WriteLine();

    initialVbeTerminal.WriteFormat("Initializing memory over entries #%us-%us...%n", (size_t)(firstMap - map)
                                                        , (size_t)(lastMap - map));
    initialVbeTerminal.WriteFormat(" Address rage: %X8-%X8.%n", start, end);
    initialVbeTerminal.WriteFormat(" Maps start at %Xp.%n", firstMap);

    initialVbeTerminal.WriteLine();

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

    size_t cpuDataSize = JG_INFO_ROOT_EX->cpu_count * Cpu::CpuDataSize;
    size_t cpuDataPageCount = (cpuDataSize + PageSize - 1) / PageSize;

    //  This is where the CPU data will sit.
    CpuDataBase = MemoryManagerAmd64::KernelModulesCursor;

    MemoryManagerAmd64::KernelModulesCursor += cpuDataPageCount << 12;
    //  The CPU data will snuggle in with the kernel modules. Whoopsie!
    //  And advance the cursor...

    for (size_t i = 0; i < cpuDataPageCount; ++i)
    {
		PageDescriptor * desc = nullptr;

		const vaddr_t vaddr = CpuDataBase + (i << 12);
		const paddr_t paddr = mainAllocator.AllocatePage(desc);

		assert(paddr != nullptr && desc != nullptr
			, "  Unable to allocate a physical page for CPU-specific data!");

		res = BootstrapMemoryManager->MapPage(vaddr, paddr, PageFlags::Global | PageFlags::Writable);

		assert(res.IsOkayResult()
			, "  Failed to map page at %Xp (%XP) for CPU-specific data: %H."
			, vaddr, paddr
			, res);

        initialVbeTerminal.WriteFormat("  Allocated page for CPU data: %Xp -> %XP.%n", vaddr, paddr);
    }

    InitializeCpuData();

    //  RELEASING APs

    bootstrapReady = true;

    initialVbeTerminal.WriteLine();

    //Beelzebub::Memory::Initialize(&mainAllocationSpace, 1);
}

void InitializeMemory()
{
    initialVbeTerminal.WriteLine();
    
    BootstrapProcessorId = CpuId();
    BootstrapProcessorId.Initialize();
    //  This is required to page all the available memory.

    //breakpoint();

    //BootstrapProcessorId.PrintToTerminal(&initialVbeTerminal);

    //breakpoint();

    initialVbeTerminal.WriteLine();

    //  TODO: Take care of the 1-MiB to 16-MiB region for ISA DMA.

    SanitizeAndInitializeMemory(JG_INFO_MMAP_EX, JG_INFO_ROOT_EX->mmap_count, JG_INFO_ROOT_EX->free_paddr);

    initialVbeTerminal.WriteLine();

    //  DUMPING CONTROL REGISTERS

    Cpu::GetCr0().PrintToTerminal(&initialVbeTerminal);
    initialVbeTerminal.WriteLine();

    initialVbeTerminal.Write("CR2: ");
    initialVbeTerminal.WriteHex64((uint64_t)Cpu::GetCr2());
    initialVbeTerminal.WriteLine();

    Cpu::GetCr3().PrintToTerminal(&initialVbeTerminal);
    initialVbeTerminal.WriteLine();

    initialVbeTerminal.Write("CR4: ");
    initialVbeTerminal.WriteHex64(Cpu::GetCr4());
    initialVbeTerminal.WriteLine();

    initialVbeTerminal.WriteLine();

    Cpu::WriteBackAndInvalidateCache();
}
