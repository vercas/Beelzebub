#include <architecture.h>
#include <entry.h>
#include <memory/manager_amd64.hpp>
#include <memory/virtual_allocator.hpp>
#include <memory/paging.hpp>
#include <system/cpu.hpp>
#include <system/cpuid.hpp>

#include <jegudiel.h>
#include <isr.h>
#include <keyboard.h>
#include <screen.h>
#include <ui.h>

#include <lapic.h>
#include <screen.h>

#include <terminals/serial.hpp>
#include <memory/page_allocator.hpp>
#include <system/exceptions.hpp>
#include <kernel.hpp>
#include <debug.hpp>
#include <math.h>
#include <cpp_support.h>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Ports;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Memory::Paging;

//uint8_t initialSerialTerminal[sizeof(SerialTerminal)];
SerialTerminal initialSerialTerminal;
PageAllocationSpace mainAllocationSpace;
PageAllocator mainAllocator;
VirtualAllocationSpace bootstrapVas;
MemoryManagerAmd64 bootstrapMemMgr;

volatile uintptr_t CpuDataBase;

volatile bool bootstrapReady = false;

CpuId Beelzebub::System::BootstrapProcessorId;

/*  Entry points  */ 

void kmain_bsp()
{
    bootstrapReady = false;

    Beelzebub::Main();
}

void kmain_ap()
{
    while (!bootstrapReady) ;
    //  Await!

    bootstrapVas.Activate();
    //  Perfectly valid solution.

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

    //  And returns it.
    return &initialSerialTerminal; // termPtr;
}

TerminalBase * InitializeTerminalMain()
{
    return &initialSerialTerminal;
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

    //initialSerialTerminal.WriteHex64((uint64_t)&SerialPort::IrqHandler);
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

        mainAllocator.PreppendAllocationSpace(space);

        /*msg("%XP-%XP;%n"
            , space->GetAllocationStart()
            , space->GetAllocationEnd());//*/

        return space;
    }
    else
    {
        new (&mainAllocationSpace) PageAllocationSpace(start, end, PageSize);
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

    msg("Initializing memory over entries #%us-%us...%n", (size_t)(firstMap - map)
                                                        , (size_t)(lastMap - map));
    msg(" Address rage: %X8-%X8.%n", start, end);
    msg(" Maps start at %Xp.%n", firstMap);

    //  Mapping more momery, if available.

    if (end > (64ULL << 30) && BootstrapProcessorId.CheckFeature(CpuFeature::Page1GB))
    {
        msg(" Available memory seems to be more than 64 GiB, and 1-GiB pages are supported.%n");

        //  More than 64 gigs of memory means we need some serious 1-gig pages!

        Cr3 cr3 = Cpu::GetCr3();
        Pml4 & pml4 = *cr3.GetPml4Ptr();
        Pml3 & pml3 = *pml4[(uint16_t)0].GetPml3Ptr();

        if (pml3[(uint16_t)0].GetPageSize())
            msg("  PDPT's first entry seems to be a 1-GiB page. Assuming all available memory is mapped!%n");
        else
        {
            msg("  PDPT's first entry is a PD. Switching to 1-GiB pages.%n");

            paddr_t oldPml2Start = pml3[(uint16_t)0].GetAddress();
            size_t oldPml2Count = 0;
            //  I'm making no assumption about the number of present PDs.

            bool goOn = true;

            for (uint16_t i = 0; i < 512; ++i)
            {
                if (pml3[i].GetPresent())
                    ++oldPml2Count;

                if (((uint64_t)i << 30) < end)
                    pml3[i] = Pml3Entry((uint64_t)i << 30, true, true, false, false, false, false, false, false, false, false);
                    //  This is a 1-GiB page!
                else
                    pml3[i].SetPresent(goOn = false);
                    //  Just makin' sure.
            }

            if (goOn)
            {
                msg("   One PDPT did not suffice.%n");

                assert(end <= ((uint64_t)oldPml2Count + 1ULL) * (512ULL << 30)
                    , "   Where did you get so much RAM..? Can't map it all!");

                for (uint16_t j = 0; j < oldPml2Count; ++j)
                {
                    goOn = true;
                    //  Reused variable!!

                    Pml3 & pml3N = *((Pml3 *)oldPml2Start + j);
                    pml4[(uint16_t)(j + 1)] = Pml4Entry((paddr_t)(&pml3N), true, true, false, false);

                    for (uint16_t i = 0; i < 512; ++i)
                    {
                        if (((uint64_t)i << 30) < end)
                            pml3N[i] = Pml3Entry((uint64_t)i << 30, true, true, false, false);
                            //  This is a 1-GiB page!
                        else
                            pml3N[i].SetPresent(goOn = false);
                            //  NOT breaking because the rest of the entries need cleanup.
                    }

                    if (!goOn)
                        break; // yourself fool
                }
            }
        }
    }

    initialSerialTerminal.WriteLine();

    //  SPACE CREATION

    for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
        if (m->available && m->length >= (2 * PageSize))
        {
            if (m->address < start && (m->address + m->length) > start)
                //  Means this entry crosses the start of free memory.
                CreateAllocationSpace(start, m->address + m->length);
            else
                CreateAllocationSpace(m->address, m->address + m->length);
        }

    //  PAGE RESERVATION

#ifdef __BEELZEBUB__DEBUG
    //mainAllocationSpace.PrintStackToTerminal(&initialSerialTerminal, true);
#endif

    for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
        if (!m->available)
        {
            res = mainAllocator.ReserveByteRange(m->address, m->length);

            assert(res.IsOkayResult() || res.IsResult(HandleResult::PagesOutOfAllocatorRange)
                , "Failed to reserve page range %XP-%XP: %H."
                , m->address, m->address + m->length
                , res);
        }

#ifdef __BEELZEBUB__DEBUG
    //mainAllocationSpace.PrintStackToTerminal(&initialSerialTerminal, true);
#endif

    //  PAGING INITIALIZATION

    msg("Constructing bootstrap virtual allocation space... ");
    new (&bootstrapVas) VirtualAllocationSpace(&mainAllocator);
    msg("Done.%n");

    msg("Bootstrapping the VAS... ");
    bootstrapVas.Bootstrap();
    msg("Done.%n");

    msg("Constructing bootstrap memory manager... ");
    new (&bootstrapMemMgr) MemoryManagerAmd64(&bootstrapVas);
    msg("Done.%n");

    BootstrapMemoryManager = &bootstrapMemMgr;

    //  CPU DATA

    size_t cpuDataSize = JG_INFO_ROOT_EX->cpu_count * Cpu::CpuDataSize;
    size_t cpuDataPageCount = (cpuDataSize + PageSize - 1) / PageSize;

    //  TODO ASAP

    //  RELEASING APs

    bootstrapReady = true;

    //  DUMPING MMAP

    initialSerialTerminal.WriteLine("IND |A|    Address     |     Length     |       End      |");

    for (uint32_t i = 0; i < cnt; i++)
    {
        jg_info_mmap_t & e = map[i];
        //  Current map.

        initialSerialTerminal.WriteHex16((uint16_t)i);
        initialSerialTerminal.Write("|");
        initialSerialTerminal.Write(e.available ? "A" : " ");
        initialSerialTerminal.Write("|");
        initialSerialTerminal.WriteHex64(e.address);
        initialSerialTerminal.Write("|");
        initialSerialTerminal.WriteHex64(e.length);
        initialSerialTerminal.Write("|");
        initialSerialTerminal.WriteHex64(e.address + e.length);
        initialSerialTerminal.WriteLine("|");
    }

    initialSerialTerminal.Write("Free memory start: ");
    initialSerialTerminal.WriteHex64(freeStart);

    initialSerialTerminal.WriteLine();
    initialSerialTerminal.WriteLine();

    //Beelzebub::Memory::Initialize(&mainAllocationSpace, 1);
}

void InitializeMemory()
{
    initialSerialTerminal.WriteLine();
    
    BootstrapProcessorId = CpuId();
    BootstrapProcessorId.Initialize();
    //  This is required to page all the available memory.

    BootstrapProcessorId.PrintToTerminal(&initialSerialTerminal);

    initialSerialTerminal.WriteLine();

    //  TODO: Take care of the 1-MiB to 16-MiB region for ISA DMA.

    SanitizeAndInitializeMemory(JG_INFO_MMAP_EX, JG_INFO_ROOT_EX->mmap_count, JG_INFO_ROOT_EX->free_paddr);

    initialSerialTerminal.WriteLine();

    //  DUMPING CONTROL REGISTERS

    Cpu::GetCr0().PrintToTerminal(&initialSerialTerminal);
    initialSerialTerminal.WriteLine();

    initialSerialTerminal.Write("CR2: ");
    initialSerialTerminal.WriteHex64((uint64_t)Cpu::GetCr2());
    initialSerialTerminal.WriteLine();

    Cpu::GetCr3().PrintToTerminal(&initialSerialTerminal);
    initialSerialTerminal.WriteLine();

    initialSerialTerminal.Write("CR4: ");
    initialSerialTerminal.WriteHex64(Cpu::GetCr4());
    initialSerialTerminal.WriteLine();

    initialSerialTerminal.WriteLine();
}
