#include <ports.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Ports;
using namespace Beelzebub::System;

ManagedSerialPort Beelzebub::Ports::COM1(0x03F8);
ManagedSerialPort Beelzebub::Ports::COM2(0x02F8);
ManagedSerialPort Beelzebub::Ports::COM3(0x03E8);
ManagedSerialPort Beelzebub::Ports::COM4(0x02E8);

/************************
	SerialPort struct
*************************/

/*	Static methods  */

void SerialPort::IrqHandler(IsrState * const state)
{
	//uint8_t reg = Cpu::In8(COM1.BasePort + 2);

	//if (0 == (reg & 1))
	//{
		//COM1.WriteNtString("COM1");
	//}
}

/*  Construction  */

void SerialPort::Initialize() const
{
	Cpu::Out8(this->BasePort + 1, 0x00);    // Disable all interrupts

	Cpu::Out8(this->BasePort + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	Cpu::Out8(this->BasePort + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	Cpu::Out8(this->BasePort + 1, 0x00);    //                  (hi byte)

	Cpu::Out8(this->BasePort + 3, 0x03);    // 8 bits, no parity, one stop bit

	Cpu::Out8(this->BasePort + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold

	Cpu::Out8(this->BasePort + 4, 0x0B);    // IRQs enabled, RTS/DSR set
	Cpu::Out8(this->BasePort + 1, 0x0F);    // Enable some interrupts
}

/*  I/O  */

uint8_t SerialPort::Read(const bool wait) const
{
	if (wait) while (!this->CanRead()) ;

	return Cpu::In8(this->BasePort);
}

void SerialPort::Write(const uint8_t val, const bool wait) const
{
	if (wait) while (!this->CanWrite()) ;

	Cpu::Out8(this->BasePort, val);
}

size_t SerialPort::ReadNtString(char * const buffer, const size_t size) const
{
	size_t i = 0;
	char c;

	do
	{
		buffer[i++] = c = this->Read(true);
	} while (c != 0 && i < size);

	return i;
}

size_t SerialPort::WriteNtString(const char * const str) const
{
	size_t i = 0, j;
	const uint16_t p = this->BasePort;

	while (str[i] != 0)
	{
		while (!this->CanWrite()) ;

		const char * tmp = str + i;

		for (j = 0; j < SerialPort::QueueSize && tmp[j] != 0; ++j)
			Cpu::Out8(p, tmp[j]);

		i += j;
	}

	return i;
}

/*******************************
	ManagedSerialPort struct
********************************/

/*	Static methods  */

void ManagedSerialPort::IrqHandler(IsrState * const state)
{
	//uint8_t reg = Cpu::In8(COM1.BasePort + 2);

	//if (0 == (reg & 1))
	//{
		COM1.WriteNtString("COM1");
	//}
}

/*  Construction  */

void ManagedSerialPort::Initialize()
{
	Cpu::Out8(this->BasePort + 1, 0x00);    // Disable all interrupts

	Cpu::Out8(this->BasePort + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	Cpu::Out8(this->BasePort + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	Cpu::Out8(this->BasePort + 1, 0x00);    //                  (hi byte)

	Cpu::Out8(this->BasePort + 3, 0x03);    // 8 bits, no parity, one stop bit

	Cpu::Out8(this->BasePort + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold

	Cpu::Out8(this->BasePort + 4, 0x0B);    // IRQs enabled, RTS/DSR set
	Cpu::Out8(this->BasePort + 1, 0x0F);    // Enable some interrupts

	this->OutputCount = 0;
}

/*  I/O  */

uint8_t ManagedSerialPort::Read(const bool wait)
{
	if (wait) while (!this->CanRead()) ;

	const int_cookie_t int_cookie = (&this->ReadLock)->Acquire();

	return Cpu::In8(this->BasePort);

	(&this->ReadLock)->Release(int_cookie);
}

void ManagedSerialPort::Write(const uint8_t val, const bool wait)
{
	if (wait)
		while (this->OutputCount >= SerialPort::QueueSize
			&& !this->CanWrite()) ;
	//	If the output count exceeds the queue size, I check whether I
	//	can write or not. If I can, the count is reset anyway.

    const int_cookie_t int_cookie = (&this->WriteLock)->Acquire();

	Cpu::Out8(this->BasePort, val);
	++this->OutputCount;

	(&this->WriteLock)->Release(int_cookie);
}

size_t ManagedSerialPort::ReadNtString(char * const buffer, const size_t size)
{
	size_t i = 0;
	char c;

    const int_cookie_t int_cookie = (&this->ReadLock)->Acquire();

	do
	{
		while (!this->CanRead()) ;

		buffer[i++] = c = Cpu::In8(this->BasePort);
	} while (c != 0 && i < size);

	(&this->ReadLock)->Release(int_cookie);

	return i;
}

size_t ManagedSerialPort::WriteNtString(const char * const str)
{
	size_t i = 0, j;
	const uint16_t p = this->BasePort;

    const int_cookie_t int_cookie = (&this->WriteLock)->Acquire();

	while (str[i] != 0)
	{
		while (this->OutputCount >= SerialPort::QueueSize
		    && !this->CanWrite()) ;

		const char * const tmp = str + i;

		for (j = 0; this->OutputCount < SerialPort::QueueSize && tmp[j] != 0; ++j, ++this->OutputCount)
			Cpu::Out8(p, tmp[j]);

		i += j;
	}

	(&this->WriteLock)->Release(int_cookie);

	return i;
}
