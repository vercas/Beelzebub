#pragma once

#include <arc/system/cpu.hpp>
#include <metaprogramming.h>

using namespace Beelzebub::System;

namespace Beelzebub { namespace Ports
{
	/**
	 * Represents a serial port
	 */
	class SerialPort
	{
	public:

		/*  Construction  */

		__bland SerialPort(const uint16_t basePort) : BasePort(basePort) { }

		//  Prepares the serial port for nominal operation.
		__bland void Initialize() const;

		/*  Properties  */

		//	Retrieves the base port of the serial port.
		__bland __forceinline uint16_t GetBasePort() const { return this->BasePort; }

		/*  I/O  */

		//	True if the serial port can be read from.
		__bland __forceinline bool CanRead() const
		{
			return 0 != (Cpu::In8(this->BasePort + 5) & 0x01);
			//	Bit 0 of the line status register.
		}

		//	True if the serial port can be written to.
		__bland __forceinline bool CanWrite() const
		{
			return 0 != (Cpu::In8(this->BasePort + 5) & 0x20);
			//	Bit 5 of the line status register.
		}

		//	Reads a byte from the serial port, optionally waiting for
		//  being able to read.
		__bland uint8_t Read(const bool wait) const;

		//  Writes a byte to the serial port, optionally waiting for
		//  being able to write.
		__bland void Write(const uint8_t val, const bool wait) const;

		//	Reads a null-terminated string from the serial port up to the
		//  given amount of characters, and returns the number of characters
		//  read, including the null-terminator if read. This method awaits
		//  for reading to be permitted.
		__bland size_t ReadNtString(char * const buffer, const size_t size) const;

		//  Writes a null-terminated string to the serial port.
		//  This method awaits.
		__bland size_t WriteNtString(const char * const str) const;

	private:

		uint16_t BasePort;
	} __attribute__((packed));

	extern SerialPort COM1;
	extern SerialPort COM2;
	extern SerialPort COM3;
	extern SerialPort COM4;
}}
