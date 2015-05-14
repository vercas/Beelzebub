#include <architecture.h>
#include <arc/entry.h>
#include <arc/memory/paging.hpp>
#include <arc/system/cpu.hpp>

#include <jegudiel.h>
#include <arc/isr.h>
#include <arc/keyboard.h>
#include <arc/screen.h>
#include <ui.h>

#include <arc/lapic.h>
#include <arc/screen.h>

#include <arc/terminals/serial.hpp>
#include <memory/page_allocator.hpp>
#include <kernel.hpp>
#include <debug.hpp>
#include <math.h>
#include <cpp_support.h>

using namespace Beelzebub;
using namespace Beelzebub::Ports;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Memory::Paging;

//uint8_t initialSerialTerminal[sizeof(SerialTerminal)];
SerialTerminal initialSerialTerminal;
PageAllocationSpace mainAllocationSpace;
PageAllocator mainAllocator;

static __bland void fault_gp(isr_state_t * state)
{
    //write_serial_str(0x3F8, "OMG GP FAULT!");
    //write_serial(0x3F8, '\n');

    initialSerialTerminal.WriteFormat("ISR Handler: %X8%n", state->vector);
}

/*  Entry points  */ 

void kmain_bsp()
{
    Beelzebub::Main();
}

void kmain_ap()
{
    Beelzebub::Secondary();
}

/*  Terminal  */

TerminalBase * InitializeTerminalProto()
{
    //  TODO: Properly retrieve these addresses.

    //  Initializes COM1.
    COM1 = ManagedSerialPort(0x3F8);
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
        isr_handlers[i] = (uintptr_t)&fault_gp;
    }

    isr_handlers[32] = (uintptr_t)&SerialPort::IrqHandler;

    initialSerialTerminal.WriteHex64((uint64_t)&SerialPort::IrqHandler);
}

/**********************************************/
/*  Memory map sanitation and initialization  */
/**********************************************/

bool firstRegionCreated;

PageAllocationSpace * CreateAllocationSpace(uint64_t start, uint64_t end)
{
    if (firstRegionCreated)
    {

    }
    else
    {
        new (&mainAllocationSpace) PageAllocationSpace(start, end, PageSize);
        new (&mainAllocator)       PageAllocator(&mainAllocationSpace);

        firstRegionCreated = true;

        return &mainAllocationSpace;
    }

    return nullptr;
}

void SanitizeAndInitializeMemory(jg_info_mmap_t * map, uint32_t cnt, uintptr_t freeStart)
{
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

        uint64_t addressMisalignment = RoundUpDiff(m->address, PageSize);
        //  The address is rounded up to the closest page;

        m->address += addressMisalignment;
        m->length -= addressMisalignment;

        m->length -= m->length % PageSize;
        //  The length is rounded down.

        uint64_t mEnd = m->address + m->length;

        if (mEnd > end)
        {
            end = mEnd;
            lastMap = m;
        }
    }

    msg("Initializing memory over entries #%us-%us...", (size_t)(firstMap - map)
                                                      , (size_t)(lastMap - map));
    msg(" Address rage: %X8-%X8.%n", start, end);

    //uint64_t maxGapSize = 2 * PageSize * PageSize / sizeof(PageDescriptor);
    //  The maximum size of a region of reserved memory.
    //  Any greater and it is considered to split memory regions.
    //  ... Because it would waste space with descriptors and generally
    //  slow down contigious allocations.

    firstRegionCreated = false;

    bool lastAvailable = true;
    uint64_t availableEnd = lastMap->address + lastMap->length, reservedEnd;
    uint64_t availableStart = lastMap->address, reservedStart;

    msg("-- Initial range: %X8-%X8;%n", availableStart, availableEnd);

    for (jg_info_mmap_t * m = lastMap - 1; m >= firstMap; --m)
    {
        uint64_t start = m->address, end = m->address + m->length;
        bool available = 0 != m->available;

        msg("-- Range: (%c) %X8-%X8: L%c"
            , available ? 'A' : 'R'
            , start, end
            , lastAvailable ? 'A' : 'R');

        if (available)
        {
            if (lastAvailable)
            {
                assert(end == availableStart
                    , "The end of the current available entry doesn't match the start of the previous available entry.");

                availableStart = start;

                msg(" EXTENDING AVAILABLE;%n");
            }
            else
            {
                assert(end == reservedStart
                    , "The end of the current available entry doesn't match the start of the previous reserved entry.");

                msg(" STARTING AVAILABLE;%n");
            }
        }
        else
        {
            if (lastAvailable)
            {
                assert(end == availableStart
                    , "The end of the current reserved entry doesn't match the start of the previous available entry.");

                reservedStart = start;
                reservedEnd = end;

                msg(" STARTING RESERVED;%n");
            }
            else
            {
                assert(end == reservedStart
                    , "The end of the current reserved entry doesn't match the start of the previous reserved entry.");

                reservedStart = start;

                msg(" EXTENDING RESERVED;%n");
            }
        }

        lastAvailable = available;
    }

    new (&mainAllocationSpace) PageAllocationSpace(start, end, PageSize);
    new (&mainAllocator)       PageAllocator(&mainAllocationSpace);

#ifdef __BEELZEBUB__DEBUG
    //mainAllocationSpace.PrintStackToTerminal(&initialSerialTerminal, true);
#endif

    for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
        if (!m->available)
        {
            Handle res = mainAllocationSpace.ReserveByteRange(m->address, m->length);

            assert(res.IsOkayResult()
                , "Failed to reserve page range %XP-%XP: %H."
                , m->address, m->address + m->length
                , res);
        }

#ifdef __BEELZEBUB__DEBUG
    //mainAllocationSpace.PrintStackToTerminal(&initialSerialTerminal, true);
#endif

    //Beelzebub::Memory::Initialize(&mainAllocationSpace, 1);
}

void InitializeMemory()
{
    initialSerialTerminal.WriteLine();

    //  TODO: Take care of the 1-MiB to 16-MiB region for ISA DMA.

    SanitizeAndInitializeMemory(JG_INFO_MMAP, JG_INFO_ROOT->mmap_count, JG_INFO_ROOT->free_paddr);

    initialSerialTerminal.WriteLine();

    //  DUMPING MMAP

    initialSerialTerminal.WriteLine("IND |A|    Address     |     Length     |");

    // get pagez lol

    jg_info_mmap_t * mmap = JG_INFO_MMAP;
    uint32_t mmapcnt = JG_INFO_ROOT->mmap_count;

    for (uint32_t i = 0; i < mmapcnt; i++)
    {
        jg_info_mmap_t & e = mmap[i];
        //  Current map.

        initialSerialTerminal.WriteHex16((uint16_t)i);
        initialSerialTerminal.Write("|");
        initialSerialTerminal.Write(e.available ? "A" : " ");
        initialSerialTerminal.Write("|");
        initialSerialTerminal.WriteHex64(e.address);
        initialSerialTerminal.Write("|");
        initialSerialTerminal.WriteHex64(e.length);
        initialSerialTerminal.WriteLine("|");
    }

    initialSerialTerminal.WriteLine();

    initialSerialTerminal.WriteHex64(JG_INFO_ROOT->free_paddr);

    initialSerialTerminal.WriteLine();
    initialSerialTerminal.WriteLine();

    //  DUMPING CONTROL REGISTERS

    Cpu::GetCr0().PrintToTerminal(&initialSerialTerminal);

    initialSerialTerminal.Write("CR2: ");
    initialSerialTerminal.WriteHex64((uint64_t)Cpu::GetCr2());
    initialSerialTerminal.WriteLine();

    Cpu::GetCr3().PrintToTerminal(&initialSerialTerminal);

    initialSerialTerminal.Write("CR4: ");
    initialSerialTerminal.WriteHex64(Cpu::GetCr4());
    initialSerialTerminal.WriteLine();

    initialSerialTerminal.WriteLine();

    //  Preparing virtual memory tables

    paddr_t pml4_addr = mainAllocationSpace.AllocatePage();

    Pml4 & pml4 = * new ((Pml4 *)pml4_addr) Pml4();

    Cr3 cr3(Cpu::GetCr3());
    Pml4 & currentPml4 = *cr3.GetPml4Ptr();

    pml4[511] = currentPml4[511];
    pml4[510] = currentPml4[510];
}
