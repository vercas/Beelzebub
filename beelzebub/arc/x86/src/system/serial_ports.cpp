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

#include "system/serial_ports.hpp"
#include "system/io_ports.hpp"
#include "kernel.hpp"
#include <beel/interrupt.state.hpp>
#include <beel/terminals/base.hpp>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Utils;

ManagedSerialPort Beelzebub::System::COM1 {0x03F8};
ManagedSerialPort Beelzebub::System::COM2 {0x02F8};
ManagedSerialPort Beelzebub::System::COM3 {0x03E8};
ManagedSerialPort Beelzebub::System::COM4 {0x02E8};

/************************
    SerialPort struct
*************************/

/*  Static fields  */

size_t const SerialPort::QueueSize = 16;

// /*  Static methods  */

// void SerialPort::IrqHandler(INTERRUPT_HANDLER_ARGS)
// {
//     (void)state;

//     //COM1.WriteNtString("IRQ!");

//     END_OF_INTERRUPT();
// }

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

size_t SerialPort::WriteNtString(char const * const str, size_t len) const
{
    size_t i = 0, j, u = 0;
    uint16_t const p = this->BasePort;

    //  `u` is the number of unicode characters encountered.

    while (str[i] != 0 && likely(i < len))
    {
        while (!this->CanWrite()) ;

        size_t const left = Minimum(len - i, SerialPort::QueueSize);
        char const * const tmp = str + i;

        char c = tmp[j = 0];

        do
        {
            if likely((c & 0xC0) != 0x80)
                ++u;
            //  Upper bit is 0 means this is a one-byte character.
            //  If upper bit is 1, the one before must be 1 as well for this to
            //  be the start of a multibyte character.

            ++j;
        } while ((c = tmp[j]) != 0 && likely(j < left));

        Io::Out8n(p, tmp, j);

        i += j;
    }

    return u;
}

/*******************************
    ManagedSerialPort struct
********************************/

/*  Static methods  */

void ManagedSerialPort::IrqHandler(InterruptContext const * context, ManagedSerialPort * port)
{
    (void)context;

    uint8_t const iir = Io::In8(port->BasePort + 2);

    if (0 == (iir & 1) && port->ImmediateStatus == 0)
    {
        uint8_t const cause = (iir & 0xE) >> 1;

        if (cause == 1)
        {
            //  Needs more data!

            if (port->WriteLock.TryAcquire())
            {
                with (RingBufferConcurrent::PopCookie const cookie = port->Queue.TryBeginPop(port->QueueSize))
                {
                    if (cookie.IsValid())
                    {
                        Io::Out8n(port->BasePort, cookie.Array, cookie.Count);
                        // MainTerminal->Write('S');
                    }
                    // else
                    //     MainTerminal->Write('O');
                    //  Failing this check means another consumer obtained data.
                }

                port->WriteLock.Release();
            }
            // else
            // {
            //     MainTerminal->Write('X');
            //     MSG("SCHEISSE%X1", iir);
            // }

            //  Failure to lock here means that this interrupt was delivered late
            //  for some reason, and a producer took initiative to output data.
        }
        // else
        // {
        //     MainTerminal->Write('C');
        //     MSG("SERIAL%X1", iir);
        // }
    }
    // else
    //     MainTerminal->Write('?');
}

/*  Construction  */

ManagedSerialPort::ManagedSerialPort(uint16_t const basePort) 
    : BasePort( basePort)
    , QueueSize(16)
    , OutputCount(0)
    , Type()
    , ReadLock()
    , WriteLock()
    , Queue(this->Buffer, 1024)
    , Buffer()
    , ImmediateStatus(2)
{

}

static SerialPortType FindType(uint16_t const base)
{
    Io::Out8(base + 2, 0xE7);

    uint8_t const flags = Io::In8(base + 2);

    if (flags == 0xFF && Io::In8(base + 5) == 0xFF && Io::In8(base + 6) == 0xFF)
        return SerialPortType::Disconnected;
    //  Best I can do.

    if (0 != (flags & 0x40))
        if (0 != (flags & 0x80))
            if (0 != (flags & 0x20))
                return SerialPortType::D16750;
            else
                return SerialPortType::NS16550A;
        else
            return SerialPortType::NS16550;
    else
    {
        Io::Out8(base + 7, 0x42);

        if (Io::In8(base + 7) == 0x42)
            return SerialPortType::NS16450;
        else
            return SerialPortType::NS8250;
    }
}

void ManagedSerialPort::Initialize()
{
    if ((this->Type = FindType(this->BasePort)) == SerialPortType::Disconnected)
        return;

    Io::Out8(this->BasePort + 3, 0x03);    // Just make sure DLAB is clear.

    Io::Out8(this->BasePort + 1, 0x00);    // Eh...
    Io::Out8(this->BasePort + 1, 0x00);    // Disable all interrupts

    Io::Out8(this->BasePort + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    Io::Out8(this->BasePort + 0, 0x01);    // Set divisor to 3 (lo byte) 38400 baud
    Io::Out8(this->BasePort + 1, 0x00);    //                  (hi byte)

    Io::Out8(this->BasePort + 3, 0x03);    // 8 bits, no parity, one stop bit

    if (this->Type == SerialPortType::D16750)
    {
        Io::Out8(this->BasePort + 2, 0x27);    // Enable FIFO, clear them, with 64-byte FIFO

        this->QueueSize = 64;
    }
    else if (this->Type != SerialPortType::NS8250)
        Io::Out8(this->BasePort + 2, 0x07);    // Enable FIFO, clear them, with 16-byte FIFO
    else
        this->QueueSize = 1;                   // Ancient chip, has no FIFO.

    Io::Out8(this->BasePort + 4, 0x03);    // IRQs disabled, RTS/DSR set
    Io::Out8(this->BasePort + 1, 0x00);    // Tell it, no IRQs.

    // if (this->Type == SerialPortType::D16750)
    //     Io::Out8(this->BasePort + 2, 0x21);    // Eh?
    // else
    //     Io::Out8(this->BasePort + 2, 0x01);    // Eh!

    this->OutputCount = 0;
}

void ManagedSerialPort::EnableInterrupts()
{
    Io::Out8(this->BasePort + 1, 0x00);
    Io::Out8(this->BasePort + 1, 0x00);
    //  Some book says this works around some bug. :(

    // uint8_t iir = Io::In8(this->BasePort + 2);

    (void)Io::In8(this->BasePort + 0);
    (void)Io::In8(this->BasePort + 2);
    (void)Io::In8(this->BasePort + 5);
    (void)Io::In8(this->BasePort + 6);
    //  Poke some registers. To clear interrupts.

    Io::Out8(this->BasePort + 4, 0x0B);    // IRQs enabled, RTS/DSR set

    // char buf[] = "INIT 000";
    // for (size_t i = 7; iir > 0 && i >= 5; --i, iir /= 10) buf[i] = '0' + iir % 10;

    // this->WriteNtString(buf, 8);

    Io::Out8(this->BasePort + 1, 0x03);    // Enable data receipt interrupts.
    Io::Out8(this->BasePort + 1, 0x03);    // Enable data receipt interrupts.
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

        this->OutputCount = 0;

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
            while (this->OutputCount >= this->QueueSize
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

size_t ManagedSerialPort::WriteNtString(char const * const str, size_t len)
{
    size_t i = 0, u = 0;

    //  `u` is the number of unicode characters encountered.

    for (char c; (c = str[i]) != 0 && i < len; ++i)
        if likely((c & 0xC0) != 0x80)
            ++u;
    //  Upper bit is 0 means this is a one-byte character.
    //  If upper bit is 1, the one before must be 1 as well for this to
    //  be the start of a multibyte character.

    this->WriteBytes(str, i);

    return u;

    // size_t i = 0, j, u = 0;
    // uint16_t const p = this->BasePort;

    // //  `u` is the number of unicode characters encountered.

    // withLock (this->WriteLock)
    //     while (str[i] != 0 && likely(i < len))
    //     {
    //         while (this->OutputCount >= this->QueueSize
    //             && !this->CanWrite()) ;

    //         size_t const left = Minimum(len - i, this->QueueSize - this->OutputCount);
    //         char const * const tmp = str + i;

    //         char c = tmp[j = 0];

    //         do
    //         {
    //             if likely((c & 0xC0) != 0x80)
    //                 ++u;
    //             //  Upper bit is 0 means this is a one-byte character.
    //             //  If upper bit is 1, the one before must be 1 as well for this to
    //             //  be the start of a multibyte character.

    //             ++j;
    //         } while ((c = tmp[j]) != 0 && likely(j < left));

    //         Io::Out8n(p, tmp, j);

    //         this->OutputCount += j;
    //         i += j;
    //     }

    // return u;
}

size_t ManagedSerialPort::WriteUtf8Char(char const * str)
{
    if (*str == 0)
        return 0;
    else if ((str[0] & 0x80) == 0)
    {
        this->WriteBytes(str, 1);

        return 1;
    }
    else
    {
        size_t i = 0;

        while ((str[++i] & 0xC0) == 0x80) { /* nothing */ }

        this->WriteBytes(str, i);

        return i;
    }

    // withLock (this->WriteLock)
    //     if ((*str & 0x80) == 0)
    //     {
    //         while (this->OutputCount >= this->QueueSize
    //             && !this->CanWrite()) ;

    //         Io::Out8(this->BasePort, *str);
    //         ++this->OutputCount;

    //         return 1;
    //         //  This is a one-byte character.
    //     }
    //     else
    //     {
    //         size_t i = 0;

    //         while ((str[++i] & 0xC0) == 0x80) { }

    //         while (this->OutputCount > this->QueueSize - i
    //             && !this->CanWrite()) ;

    //         Io::Out8n(this->BasePort, str, i);

    //         this->OutputCount += i;

    //         return i;
    //         //  Returns the number of bytes in this UTF-8 character.
    //     }

    // return ~0;  //  Not reached.
}

void ManagedSerialPort::WriteBytes(void const * src, size_t cnt)
{
    InterruptGuard<> ig;

    if (this->ImmediateStatus == 0)
        for (size_t i = 0; (i = this->Queue.TryPush((uint8_t const *)src + i, cnt)) < cnt; cnt -= i, PTR_ADD(src, i))
        {
            //  Not enough data could be pushed. Maybe no core is available to consume.

            if (0 == (Io::In8(this->BasePort + 5) & 0x20))
                continue;
            //  The output FIFO is NOT empty...

            if (!this->WriteLock.TryAcquire())
                continue;
            //  Another core is in the process of writing.

            if (0 == (Io::In8(this->BasePort + 5) & 0x20))
                goto next;
            //  UART FIFO is, once again, not empty. The lock may be released.

            with (RingBufferConcurrent::PopCookie const cookie = this->Queue.TryBeginPop(this->QueueSize))
            {
                if (cookie.IsValid())
                    Io::Out8n(this->BasePort, cookie.Array, cookie.Count);
                //  Failing this check means another consumer obtained data.
            }

        next:
            this->WriteLock.Release();
        }
    else if (this->ImmediateStatus == 1)
        while (!this->Queue.IsEmpty())
            DO_NOTHING();
    else if (this->ImmediateStatus == 2)
        withLock (this->WriteLock)
            for (size_t i = 0, j; i < cnt; i += j)
            {
                while (this->OutputCount >= this->QueueSize && !this->CanWrite())
                    DO_NOTHING();

                j = Minimum(this->QueueSize - this->OutputCount, cnt - i);

                Io::Out8n(this->BasePort, (uint8_t const *)src + i, j);
                this->OutputCount += j;
            }
}

/************************
    TERMINAL PRINTING
************************/

namespace Beelzebub { namespace Terminals
{
    template<>
    TerminalBase & operator << <ManagedSerialPort const *>(TerminalBase & term, ManagedSerialPort const * const sp)
    {
        term.WriteFormat("[Serial Port %X2 (%s) | Type: %s; LSR %X1; MSR %X1]"
            , sp->BasePort
            , sp->BasePort == 0x03F8 ? "COM1"
            : sp->BasePort == 0x02F8 ? "COM2"
            : sp->BasePort == 0x03E8 ? "COM3"
            : sp->BasePort == 0x02E8 ? "COM4"
            : "UNKNOWN"
            , sp->Type == SerialPortType::Disconnected ? "Disconnected"
            : sp->Type == SerialPortType::D16750 ? "D16750"
            : sp->Type == SerialPortType::NS16550A ? "NS16550A"
            : sp->Type == SerialPortType::NS16550 ? "NS16550"
            : sp->Type == SerialPortType::NS16450 ? "NS16450"
            : "NS8250"
            , Io::In8(sp->BasePort + 5)
            , Io::In8(sp->BasePort + 6));

        return term;
    }

    template<>
    TerminalBase & operator << <ManagedSerialPort *>(TerminalBase & term, ManagedSerialPort * const reg)
    {
        return term << const_cast<ManagedSerialPort const *>(reg);
    }
}}
