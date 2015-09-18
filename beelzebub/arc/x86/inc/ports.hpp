#pragma once

#include <system/cpu.hpp>
#include <synchronization/spinlock_uninterruptible.hpp>
#include <system/isr.hpp>
#include <metaprogramming.h>

using namespace Beelzebub::System;
using namespace Beelzebub::Synchronization;

namespace Beelzebub { namespace Ports
{
    /**
     * Represents a serial port.
     */
    class SerialPort
    {
    public:

        /*  Static fields  */

        //  Size of the queue of the port.
        static const size_t QueueSize = 14;

        /*  Static methods  */

        static __bland void IrqHandler(IsrState * const state);

        /*  Construction  */

        __bland SerialPort(const uint16_t basePort) : BasePort(basePort) { }

        //  Prepares the serial port for nominal operation.
        __bland void Initialize() const;

        /*  Properties  */

        //  Retrieves the base port of the serial port.
        __bland __forceinline uint16_t GetBasePort() const { return this->BasePort; }

        /*  I/O  */

        //  True if the serial port can be read from.
        __bland __forceinline bool CanRead() const
        {
            return 0 != (Cpu::In8(this->BasePort + 5) & 0x01);
            //  Bit 0 of the line status register.
        }

        //  True if the serial port can be written to.
        __bland __forceinline bool CanWrite() const
        {
            return 0 != (Cpu::In8(this->BasePort + 5) & 0x20);
            //  Bit 5 of the line status register.
        }

        //  Reads a byte from the serial port, optionally waiting for
        //  being able to read.
        __bland uint8_t Read(const bool wait) const;

        //  Writes a byte to the serial port, optionally waiting for
        //  being able to write.
        __bland void Write(const uint8_t val, const bool wait) const;

        //  Reads a null-terminated string from the serial port up to the
        //  given amount of characters, and returns the number of characters
        //  read, including the null-terminator if read. This method awaits
        //  for reading to be permitted.
        __bland size_t ReadNtString(char * const buffer, const size_t size) const;

        //  Writes a null-terminated string to the serial port.
        //  This method awaits.
        __bland size_t WriteNtString(const char * const str) const;

    private:

        uint16_t BasePort;
    } __packed;

    /**
     * Represents a serial port whose output is managed.
     */
    class ManagedSerialPort
    {
    public:

        /*  Static fields  */

        //  Size of the queue of the port.
        static const size_t QueueSize = 14;

        /*  Static methods  */

        static __bland void IrqHandler(IsrState * const state);

        /*  Construction  */

        __bland ManagedSerialPort(const uint16_t basePort) 
            : BasePort(basePort)
            , OutputCount(0)
            , ReadLock()
            , WriteLock()
        {

        }

        //  Prepares the serial port for nominal operation.
        __bland void Initialize();

        /*  Properties  */

        //  Retrieves the base port of the serial port.
        __bland __forceinline uint16_t GetBasePort() { return this->BasePort; }

        /*  I/O  */

        //  True if the serial port can be read from.
        __bland __forceinline bool CanRead() const
        {
            return 0 != (Cpu::In8(this->BasePort + 5) & 0x01);
            //  Bit 0 of the line status register.
        }

        //  True if the serial port can be written to.
        //  Also resets the output count if possible.
        __bland __forceinline bool CanWrite()
        {
            if (0 != (Cpu::In8(this->BasePort + 5) & 0x20))
            {
                //  Bit 5 of the line status register.

                this->OutputCount = 0;

                return true;
            }
            else
                return false;
        }

        //  Reads a byte from the serial port, optionally waiting for
        //  being able to read.
        __bland uint8_t Read(const bool wait);

        //  Writes a byte to the serial port, optionally waiting for
        //  being able to write.
        __bland void Write(const uint8_t val, const bool wait);

        //  Reads a null-terminated string from the serial port up to the
        //  given amount of characters, and returns the number of characters
        //  read, including the null-terminator if read. This method awaits
        //  for reading to be permitted.
        __bland size_t ReadNtString(char * const buffer, const size_t size);

        //  Writes a null-terminated string to the serial port.
        //  This method awaits.
        __bland size_t WriteNtString(const char * const str);

    private:

        /*  Fields  */

        uint16_t BasePort;
        uint16_t  OutputCount;

        SpinlockUninterruptible ReadLock;
        SpinlockUninterruptible WriteLock;
    };

    extern ManagedSerialPort COM1;
    extern ManagedSerialPort COM2;
    extern ManagedSerialPort COM3;
    extern ManagedSerialPort COM4;
}}
