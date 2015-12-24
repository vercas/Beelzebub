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

#include <system/serial_ports.hpp>
#include <system/io_ports.hpp>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::System;

ManagedSerialPort Beelzebub::System::COM1 {0x03F8};
ManagedSerialPort Beelzebub::System::COM2 {0x02F8};
ManagedSerialPort Beelzebub::System::COM3 {0x03E8};
ManagedSerialPort Beelzebub::System::COM4 {0x02E8};

/************************
    SerialPort struct
*************************/

/*  Static methods  */

void SerialPort::IrqHandler(INTERRUPT_HANDLER_ARGS)
{
    //COM1.WriteNtString("IRQ!");

    END_OF_INTERRUPT();
}

/*  Construction  */

void SerialPort::Initialize() const
{
    Io::Out8(this->BasePort + 1, 0x00);    // Disable all interrupts

    Io::Out8(this->BasePort + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    Io::Out8(this->BasePort + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    Io::Out8(this->BasePort + 1, 0x00);    //                  (hi byte)

    Io::Out8(this->BasePort + 3, 0x03);    // 8 bits, no parity, one stop bit

    Io::Out8(this->BasePort + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold

    Io::Out8(this->BasePort + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    //Io::Out8(this->BasePort + 1, 0x0F);    // Enable some interrupts
}

/*  I/O  */

bool SerialPort::CanRead() const
{
    return 0 != (Io::In8(this->BasePort + 5) & 0x01);
    //  Bit 0 of the line status register.
}

bool SerialPort::CanWrite() const
{
    return 0 != (Io::In8(this->BasePort + 5) & 0x20);
    //  Bit 5 of the line status register.
}

uint8_t SerialPort::Read(bool const wait) const
{
    if (wait) while (!this->CanRead()) ;

    return Io::In8(this->BasePort);
}

void SerialPort::Write(uint8_t const val, bool const wait) const
{
    if (wait) while (!this->CanWrite()) ;

    Io::Out8(this->BasePort, val);
}

size_t SerialPort::ReadNtString(char * const buffer, size_t const size) const
{
    size_t i = 0;
    char c;

    do
    {
        buffer[i++] = c = this->Read(true);
    } while (c != 0 && i < size);

    return i;
}

size_t SerialPort::WriteNtString(char const * const str) const
{
    size_t i = 0, j;
    uint16_t const p = this->BasePort;

    while (str[i] != 0)
    {
        while (!this->CanWrite()) ;

        const char * tmp = str + i;

        for (j = 0; j < SerialPort::QueueSize && tmp[j] != 0; ++j)
            Io::Out8(p, tmp[j]);

        i += j;
    }

    return i;
}

/*******************************
    ManagedSerialPort struct
********************************/

/*  Static methods  */

void ManagedSerialPort::IrqHandler(IsrState * const state)
{
    //uint8_t reg = Io::In8(COM1.BasePort + 2);

    //if (0 == (reg & 1))
    //{
        COM1.WriteNtString("COM1");
    //}
}

/*  Construction  */

void ManagedSerialPort::Initialize()
{
    Io::Out8(this->BasePort + 1, 0x00);    // Disable all interrupts

    Io::Out8(this->BasePort + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    Io::Out8(this->BasePort + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    Io::Out8(this->BasePort + 1, 0x00);    //                  (hi byte)

    Io::Out8(this->BasePort + 3, 0x03);    // 8 bits, no parity, one stop bit

    Io::Out8(this->BasePort + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold

    Io::Out8(this->BasePort + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    Io::Out8(this->BasePort + 1, 0x0F);    // Enable some interrupts

    this->OutputCount = 0;
}

/*  I/O  */

bool ManagedSerialPort::CanRead() const
{
    return 0 != (Io::In8(this->BasePort + 5) & 0x01);
    //  Bit 0 of the line status register.
}

bool ManagedSerialPort::CanWrite()
{
    if (0 != (Io::In8(this->BasePort + 5) & 0x20))
    {
        //  Bit 5 of the line status register.

        this->OutputCount.Store(0);

        return true;
    }
    else
        return false;
}

uint8_t ManagedSerialPort::Read(bool const wait)
{
    if (wait) while (!this->CanRead()) ;

    withLock (this->ReadLock)
        return Io::In8(this->BasePort);

    return ~0;
    //  Won't get executed.
}

void ManagedSerialPort::Write(uint8_t const val, bool const wait)
{
    withLock (this->WriteLock)
    {
        if (wait)
            while (this->OutputCount.Load() >= SerialPort::QueueSize
                && !this->CanWrite()) ;
        //  If the output count exceeds the queue size, I check whether I
        //  can write or not. If I can, the count is reset anyway.

        Io::Out8(this->BasePort, val);
        ++this->OutputCount;
    }
}

size_t ManagedSerialPort::ReadNtString(char * const buffer, size_t const size)
{
    size_t i = 0;
    char c;

    withLock (this->ReadLock)
        do
        {
            while (!this->CanRead()) ;

            buffer[i++] = c = Io::In8(this->BasePort);
        } while (c != 0 && i < size);

    return i;
}

size_t ManagedSerialPort::WriteNtString(char const * const str)
{
    size_t i = 0, j, u = 0;
    uint16_t const p = this->BasePort;

    //  `u` is the number of unicode characters encountered.

    withLock (this->WriteLock)
        while (str[i] != 0)
        {
            while (this->OutputCount.Load() >= SerialPort::QueueSize
                && !this->CanWrite()) ;

            char const * const tmp = str + i;

            for (j = 0; this->OutputCount < SerialPort::QueueSize && tmp[j] != 0; ++j, ++this->OutputCount)
            {
                char const c = tmp[j];

                Io::Out8(p, c);

                if ((c & 0x80) == 0 || (c & 0x40) != 0)
                    ++u;
                //  Upper bit is 0 means this is a one-byte character.
                //  If upper bit is 1, the one before must be 1 as well for this to
                //  be the start of a multibyte character.
            }

            i += j;
        }

    return u;
}

size_t ManagedSerialPort::WriteUtf8Char(char const * str)
{
    if (*str == 0)
        return 0;

    withLock (this->WriteLock)
        if ((*str & 0x80) == 0)
        {
            while (this->OutputCount.Load() >= SerialPort::QueueSize
                && !this->CanWrite()) ;

            Io::Out8(this->BasePort, *str);
            ++this->OutputCount;

            return 1;
        }
        else
        {
            while (this->OutputCount.Load() > SerialPort::QueueSize - 6
                && !this->CanWrite()) ;
            //  It seems that 6 is the maximum length of a UTF-8 character..?

            size_t i = 0;

            do
            {
                Io::Out8(this->BasePort, str[i++]);
            } while ((str[i] & 0xC0) == 0x80);
            //  Condition checks for continuation bytes.
            //  Also, note that `i` is post-incremented there. The condition
            //  will check the byte after the one which is output.

            this->OutputCount += i;

            return i;
        }

    return ~0;
}

void ManagedSerialPort::WriteBytes(void const * const src, size_t const cnt)
{
    uint16_t const p = this->BasePort;

    withLock (this->WriteLock)
        for (size_t i = 0, j; i < cnt; i += j)
        {
            while (this->OutputCount.Load() >= SerialPort::QueueSize
                && !this->CanWrite()) { }

            j = Minimum(SerialPort::QueueSize - this->OutputCount.Load(), cnt - i);

            Io::Out8n(p, (uint8_t const *)src + i, j);
            this->OutputCount += j;
        }
}
