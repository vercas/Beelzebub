#include <architecture.h>
#include <arc/entry.h>
#include <arc/memory/paging.hpp>
#include <arc/cpu.hpp>

#include <jegudiel.h>
#include <arc/isr.h>
#include <arc/keyboard.h>
#include <arc/screen.h>
#include <ui.h>

#include <arc/lapic.h>
#include <arc/screen.h>

#include <arc/terminals/serial.hpp>
#include <memory.hpp>
#include <kernel.hpp>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::Ports;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Memory::Paging;

//uint8_t initialSerialTerminal[sizeof(SerialTerminal)];
SerialTerminal initialSerialTerminal;
PageAllocationSpace mainAllocationSpace;

static __bland void fault_gp(isr_state_t * state)
{
	//write_serial_str(0x3F8, "OMG GP FAULT!");
	//write_serial(0x3F8, '\n');
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

TerminalBase * InitializeTerminalMain()
{
	//	TODO: Properly retrieve these addresses.

	//	Initializes COM1.
	COM1 = SerialPort(0x3F8);
	COM1.Initialize();

	//	Initializes the serial terminal.
	initialSerialTerminal = SerialTerminal(COM1);

	//	And returns it.
	return &initialSerialTerminal; // termPtr;
}

TerminalBase * InitializeTerminalSecondary()
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
}

/*  Memory map sanitation and initialization  */

void SanitizeAndInitializeMemory(jg_info_mmap_t * map, uint32_t cnt, uintptr_t freeStart)
{
	//  First step is aligning the memory map.
	//  Also, yes, I could've used bits 'n powers of two here.
	//  But I'm hoping to future-proof the code a bit, in case
	//  I would ever target a platform whose page size is not
	//  A power of two.
	//  Moreover, this code doesn't need to be lightning-fast. :L

	uintptr_t start = RoundUp(freeStart, PageSize),
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

		uintptr_t addressMisalignment = RoundUpDiff(m->address, PageSize);
		//  The address is rounded up to the closest page;

		m->address += addressMisalignment;
		m->length -= addressMisalignment;

		m->length -= m->length % PageSize;
		//  The length is rounded down.

		uintptr_t mEnd = m->address + m->length;

		if (mEnd > end)
		{
			end = mEnd;
			lastMap = m;
		}
	}

	mainAllocationSpace = PageAllocationSpace(start, end, PageSize);

	for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
		if (!m->available)
			mainAllocationSpace.ReserveByteRange(m->address, m->length);

	Beelzebub::Memory::Initialize(&mainAllocationSpace, 1);
}

void InitializeMemory()
{
	SanitizeAndInitializeMemory(JG_INFO_MMAP, JG_INFO_ROOT->mmap_count, JG_INFO_ROOT->free_paddr);

	initialSerialTerminal.WriteLine("");

	//	DUMPING CONTROL REGISTERS

	initialSerialTerminal.Write("CR0: ");
	initialSerialTerminal.WriteHex64(Cpu::GetCr0());
	initialSerialTerminal.WriteLine("");

	initialSerialTerminal.Write("CR2: ");
	initialSerialTerminal.WriteHex64(Cpu::GetCr2());
	initialSerialTerminal.WriteLine("");

	initialSerialTerminal.Write("CR3: ");
	initialSerialTerminal.WriteHex64(Cpu::GetCr3());
	initialSerialTerminal.WriteLine("");

	initialSerialTerminal.Write("CR4: ");
	initialSerialTerminal.WriteHex64(Cpu::GetCr4());
	initialSerialTerminal.WriteLine("");

	initialSerialTerminal.WriteLine("");

	//	DUMPING PAGING TABLES

	Cr3 cr3(Cpu::GetCr3());
	Pml4 & pml4 = *cr3.GetPml4Ptr();

	initialSerialTerminal.WriteLine("PML4:>  Address  |NXB|R/W|U/S|PWT|PCD|ACC|");

	for (size_t i = 0; i < 512; ++i)
	{
		Pml4Entry e = pml4[i];

		if (e.GetPresent())
		{
			initialSerialTerminal.WriteHex64((uint64_t)e.GetPml3Ptr());
			initialSerialTerminal.Write(" | ");
			initialSerialTerminal.Write(e.GetXd() ? "X" : " ");
			initialSerialTerminal.Write(" | ");
			initialSerialTerminal.Write(e.GetWritable() ? "X" : " ");
			initialSerialTerminal.Write(" | ");
			initialSerialTerminal.Write(e.GetUsermode() ? "X" : " ");
			initialSerialTerminal.Write(" | ");
			initialSerialTerminal.Write(e.GetPwt() ? "X" : " ");
			initialSerialTerminal.Write(" | ");
			initialSerialTerminal.Write(e.GetPcd() ? "X" : " ");
			initialSerialTerminal.Write(" | ");
			initialSerialTerminal.Write(e.GetAccessed() ? "X" : " ");
			initialSerialTerminal.WriteLine(" |");
		}
		else
			initialSerialTerminal.WriteLine("-----------------+---+---+---+---+---+---");
	}

	initialSerialTerminal.WriteLine("");
}
