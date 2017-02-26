/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#pragma once

#include "system/interrupts.hpp"
#include <beel/sync/smp.lock.hpp>
#include <beel/sync/atomic.hpp>

namespace Beelzebub { namespace System
{
    enum class SerialPortType
    {
        Disconnected,
        NS8250,
        NS16450,
        NS16550,
        NS16550A,
        D16750,
    };

    /**
     * Represents a serial port.
     */
    class SerialPort
    {
    public:

        /*  Static fields  */

        //  Size of the queue of the port.
        static size_t const QueueSize;

        /*  Static methods  */

        static void IrqHandler(INTERRUPT_HANDLER_ARGS);

        /*  Construction  */

        inline explicit constexpr SerialPort(const uint16_t basePort)
            : BasePort(basePort)
        {

        }

        //  Prepares the serial port for nominal operation.
        void Initialize() const;

        /*  I/O  */

        //  True if the serial port can be read from.
        bool CanRead() const;

        //  True if the serial port can be written to.
        bool CanWrite() const;

        //  Reads a byte from the serial port, optionally waiting for
        //  being able to read.
        uint8_t Read(bool const wait) const;

        //  Writes a byte to the serial port, optionally waiting for
        //  being able to write.
        void Write(uint8_t const val, bool const wait) const;

        //  Reads a null-terminated string from the serial port up to the
        //  given amount of characters, and returns the number of characters
        //  read, including the null-terminator if read. This method awaits
        //  for reading to be permitted.
        size_t ReadNtString(char * const buffer, const size_t size) const;

        //  Writes a null-terminated string to the serial port.
        //  This method awaits.
        size_t WriteNtString(char const * const str, size_t len = SIZE_MAX) const;

        //  Writes a unicode character.
        //  This method awaits.
        size_t WriteUtf8Char(char const * str) const;

        /*  Fields  */

        uint16_t const BasePort;
    } __packed;

    /**
     * Represents a serial port whose output is managed.
     */
    class ManagedSerialPort
    {
    public:
        /*  Static methods  */

        static void IrqHandler(INTERRUPT_HANDLER_ARGS);

        /*  Construction  */

        inline explicit constexpr ManagedSerialPort(const uint16_t basePort) 
            : BasePort( basePort)
            , QueueSize(16)
            , Type()
            , OutputCount(0)
            , ReadLock()
            , WriteLock()
        {

        }

        //  Prepares the serial port for nominal operation.
        void Initialize();

        /*  I/O  */

        //  True if the serial port can be read from.
        bool CanRead() const;

        //  True if the serial port can be written to.
        //  Also resets the output count if possible.
        bool CanWrite();

        //  Reads a byte from the serial port, optionally waiting for
        //  being able to read.
        uint8_t Read(bool const wait);

        //  Writes a byte to the serial port, optionally waiting for
        //  being able to write.
        void Write(uint8_t const val, bool const wait);

        //  Reads a null-terminated string from the serial port up to the
        //  given amount of characters, and returns the number of characters
        //  read, including the null-terminator if read. This method awaits
        //  for reading to be permitted.
        size_t ReadNtString(char * const buffer, const size_t size);

        //  Writes a null-terminated string to the serial port.
        //  This method awaits.
        size_t WriteNtString(char const * const str, size_t len = SIZE_MAX);

        //  Writes a unicode character.
        //  This method awaits.
        size_t WriteUtf8Char(char const * str);

        void WriteBytes(void const * const src, size_t const cnt);

        /*  Fields  */

        uint16_t const BasePort;
        size_t QueueSize;

        SerialPortType Type;

    private:

        uint16_t OutputCount;

        Synchronization::SmpLockUni ReadLock;
        Synchronization::SmpLockUni WriteLock;
    };

    extern ManagedSerialPort COM1;
    extern ManagedSerialPort COM2;
    extern ManagedSerialPort COM3;
    extern ManagedSerialPort COM4;
}}
