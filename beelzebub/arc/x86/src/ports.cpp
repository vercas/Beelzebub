#include <arc/ports.hpp>

using namespace Beelzebub::Ports;

SerialPort Beelzebub::Ports::COM1 = 0x03F8;
SerialPort Beelzebub::Ports::COM2 = 0x02F8;
SerialPort Beelzebub::Ports::COM3 = 0x03E8;
SerialPort Beelzebub::Ports::COM4 = 0x02E8;

/************************
	SerialPort struct
*************************/

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
