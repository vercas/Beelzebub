#include <architecture.h>
#include <arc/entry.h>

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

//uint8_t initialSerialTerminalSpace[sizeof(SerialTerminal)];
SerialTerminal initialSerialTerminalSpace;
PageAllocationSpace mainAllocationSpace;

void outb(const uint16_t port, uint8_t value)
{
    asm volatile ("outb %1, %0" :: "dN" (port), "a" (value));
}

uint8_t inb(const uint16_t port)
{
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a" (value) : "dN" (port));
    return value;
}

int is_transmit_empty(const uint16_t port)
{
	return inb(port + 5) & 0x20;
}

void write_serial(const uint16_t port, const char a)
{
	while (!is_transmit_empty(port));

	outb(port, a);
}

void write_serial_str(const uint16_t port, const char * const a)
{
	size_t i = 0;

	while (a[i] != 0)
		write_serial(port, a[i++]);
}

void write_serial_uh(const uint16_t port, const uint64_t x, const size_t d)
{
	int32_t i;

	for (i = (int32_t)d - 1; i >= 0; --i)
	{
		uint8_t nib = (x >> (i * 4)) & 0x0F;

		write_serial(port, (nib > 9 ? '7' : '0') + nib);
		//	'7' + 10 = 'A' in ASCII, dawg.
	}
}

void write_serial_str_hex(const uint16_t port, const uint8_t * const a, const size_t d)
{
	size_t i;

	for (i = 0; i < d; ++i)
	{
		write_serial_uh(port, a[i], 2);

		if (i % 4 == 3)
			write_serial(port, ' ');
		if (i % 2 == 1)
			write_serial(port, ' ');
	}
}

static __bland void fault_gp(isr_state_t * state)
{
	//write_serial_str(0x3F8, "OMG GP FAULT!");
	//write_serial(0x3F8, '\n');
}

class beis
{
	public: virtual void doDis() { write_serial_str(0x3F8, "in base class!\n"); }
};

class dereev : public beis
{
	public: virtual void doDis() override { write_serial_str(0x3F8, "in derived class!\n"); }
};

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
	write_serial(0x3F8, '\n');
	write_serial_uh(0x3F8, COM1.GetBasePort(), 4);
	write_serial(0x3F8, '\n');

	//	Initializes COM1.
	COM1 = SerialPort(0x3F8);
	COM1.Initialize();

	write_serial_uh(0x3F8, 0x3F8, 4);
	write_serial(0x3F8, '\n');
	write_serial_uh(0x3F8, COM1.GetBasePort(), 4);
	write_serial(0x3F8, '\n');

	write_serial_uh(0x3F8, sizeof(SerialTerminal), 4);
	write_serial(0x3F8, '\n');
	write_serial_str_hex(0x3F8, (uint8_t *)&initialSerialTerminalSpace, sizeof(SerialTerminal));
	write_serial(0x3F8, '\n');

	//	Initializes the serial terminal.
	//SerialTerminal * termPtr = (SerialTerminal *)initialSerialTerminalSpace;
	//*termPtr = SerialTerminal(COM1);
	initialSerialTerminalSpace = SerialTerminal(COM1);

	write_serial_str_hex(0x3F8, (uint8_t *)&initialSerialTerminalSpace, sizeof(SerialTerminal));
	write_serial(0x3F8, '\n');
	//write_serial_uh(0x3F8, &initialSerialTerminalSpace.WriteLine, 4);
	//write_serial(0x3F8, '\n');

	COM1.WriteNtString("Teeeeeest");

	//	And returns it.
	return &initialSerialTerminalSpace; // termPtr;
}

TerminalBase * InitializeTerminalSecondary()
{
	write_serial_str(0x3F8, "second terminal lol");
	write_serial(0x3F8, '\n');

	return (SerialTerminal *)&initialSerialTerminalSpace;
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
}
