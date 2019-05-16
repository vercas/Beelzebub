/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

#include "djinn.arc.hpp"
#include "utils/port.parse.hpp"
#include "system/serial_ports.hpp"
#include "system/io_ports.hpp"
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::Utils;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

ManagedSerialPort * DjinnPort = nullptr;

__hot __unsanitized DJINN_SEND_RES DjinnSendPacketThroughSerialPort(void const * packet, size_t size);
__hot __unsanitized DJINN_POLL_RES DjinnPollPacketFromSerialPort(void * buffer, size_t capacity, size_t * len);

__hot __unsanitized DJINN_SEND_RES DjinnSendPacketThroughSerialPortBase64(void const * packet, size_t size);
__hot __unsanitized DJINN_POLL_RES DjinnPollPacketFromSerialPortBase64(void * buffer, size_t capacity, size_t * len);

__unsanitized Handle Beelzebub::InitializeDebuggerInterface(DjinnInterfaces iface)
{
    bool b64 = false;

    switch (iface)
    {
    case DjinnInterfaces::COM1Base64:
        b64 = true;
        [[fallthrough]];
    case DjinnInterfaces::COM1:
        DjinnPort = &COM1;
        break;

    case DjinnInterfaces::COM2Base64:
        b64 = true;
        [[fallthrough]];
    case DjinnInterfaces::COM2:
        DjinnPort = &COM2;
        break;

    case DjinnInterfaces::COM3Base64:
        b64 = true;
        [[fallthrough]];
    case DjinnInterfaces::COM3:
        DjinnPort = &COM3;
        break;

    case DjinnInterfaces::COM4Base64:
        b64 = true;
        [[fallthrough]];
    case DjinnInterfaces::COM4:
        DjinnPort = &COM4;
        break;

    default:
        return HandleResult::NotImplemented;
    }

    DjinnInitData id {};

    if (DjinnPort != nullptr)
    {
        if (b64)
        {
            id.Sender = &DjinnSendPacketThroughSerialPortBase64;
            id.Poller = &DjinnPollPacketFromSerialPortBase64;
            id.PacketMaxSize = 1024;    //  Arbitrary number, it doesn't matter.
        }
        else
        {
            id.Sender = &DjinnSendPacketThroughSerialPort;
            id.Poller = &DjinnPollPacketFromSerialPort;
            id.PacketMaxSize = DjinnPort->QueueSize - 1;
        }
    }
    else
        return HandleResult::Failed;

    DJINN_INIT_RES res = DjinnInitialize(&id);

    if (res != DJINN_INIT_SUCCESS)
        return HandleResult::Failed;

    return HandleResult::Okay;
}

/*******************
    Serial Ports
*******************/

DJINN_SEND_RES DjinnSendPacketThroughSerialPort(void const * packet, size_t size)
{
    if (size > 0xFF)
        return DJINN_SEND_PACKET_SIZE_OOR;

    LockGuard<SmpLock> lg { DjinnPort->WriteLock };

    if unlikely(DjinnPort->OutputCount >= DjinnPort->QueueSize && !DjinnPort->CanWrite())
        return DJINN_SEND_AWAIT;

    Io::Out8(DjinnPort->BasePort, (uint8_t)size);
    DjinnPort->OutputCount++;

    for (size_t i = 0, j; i < size; i += j)
    {
        while unlikely(DjinnPort->OutputCount >= DjinnPort->QueueSize && !DjinnPort->CanWrite())
            DO_NOTHING();

        j = Minimum(DjinnPort->QueueSize - DjinnPort->OutputCount, size - i);

        Io::Out8n(DjinnPort->BasePort, (uint8_t const *)packet + i, j);
        DjinnPort->OutputCount += j;
    }

    return DJINN_SEND_SUCCESS;
}

DJINN_POLL_RES DjinnPollPacketFromSerialPort(void * buffer, size_t capacity, size_t * len)
{
    if (!DjinnPort->CanRead())
        return DJINN_POLL_NOTHING;

    uint8_t const sz = Io::In8(DjinnPort->BasePort);

    if (sz > capacity)
        return DJINN_POLL_PACKET_SIZE_OOR;

    *len = sz;
    uint8_t i = 0;

    do
    {
        while (!DjinnPort->CanRead()) DO_NOTHING();

        ((uint8_t *)buffer)[i] = Io::In8(DjinnPort->BasePort);
    } while (++i < sz);

    return DJINN_POLL_SUCCESS;
}

/**************************************
    Serial Ports w/ Base64 Encoding
**************************************/

static uint8_t const Base64Chars[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static __unsanitized void WriteByte(uint8_t const val)
{
    while (DjinnPort->OutputCount >= DjinnPort->QueueSize
        && !DjinnPort->CanWrite()) ;

    Io::Out8(DjinnPort->BasePort, val);
    ++DjinnPort->OutputCount;
}

DJINN_SEND_RES DjinnSendPacketThroughSerialPortBase64(void const * packet, size_t size)
{
    if unlikely(DjinnPort->OutputCount >= DjinnPort->QueueSize && !DjinnPort->CanWrite())
        return DJINN_SEND_AWAIT;

    uint8_t const * in = reinterpret_cast<uint8_t const *>(packet);
    uint8_t const * const end = in + size;

    LockGuard<SmpLock> lg { DjinnPort->WriteLock };

    for (/* nothing */; end - in >= 3; in += 3)
    {
        WriteByte(Base64Chars[in[0] >> 2]);
        WriteByte(Base64Chars[((in[0] & 0x03) << 4) | (in[1] >> 4)]);
        WriteByte(Base64Chars[((in[1] & 0x0f) << 2) | (in[2] >> 6)]);
        WriteByte(Base64Chars[in[2] & 0x3f]);
    }

    if (end > in)
    {
        WriteByte(Base64Chars[in[0] >> 2]);

        if (end - in == 1)
        {
            WriteByte(Base64Chars[(in[0] & 0x03) << 4]);
            WriteByte('=');
        }
        else
        {
            WriteByte(Base64Chars[((in[0] & 0x03) << 4) | (in[1] >> 4)]);
            WriteByte(Base64Chars[(in[1] & 0x0f) << 2]);
        }

        WriteByte('=');
    }

    WriteByte('\n');    //  When using PTY-backed serial ports, this should flush nicely.

    return DJINN_SEND_SUCCESS;
}

static uint32_t const Base64Values[256] = {
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63,
52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
 0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0, 63,
 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

DJINN_POLL_RES DjinnPollPacketFromSerialPortBase64(void * buffer, size_t capacity, size_t * len)
{
    if (!DjinnPort->CanRead())
        return DJINN_POLL_NOTHING;

    uint8_t temp[4];
    size_t i = 0;
    int j;

request_chars:
    j = 0;

    do
    {
        while (!DjinnPort->CanRead()) DO_NOTHING();

        temp[j] = Io::In8(DjinnPort->BasePort);
    } while (temp[j] != '\n' && ++j < 4);

    if (j == 0)
    {
        //  K being 0 means a newline was read into the first slot of the buffer.
        //  Also this packet has no padding.

        *len = i;
        return DJINN_POLL_SUCCESS;
    }

    //  j has to be 4 here, otherwise the base64 data isn't encoded properly.

    if (temp[2] == '=')
    {
        //  This means there's two padding sextets, so just one byte.

        if ((*len = i + 1) > capacity)
            return DJINN_POLL_PACKET_SIZE_OOR;

        uint32_t const n = Base64Values[temp[0]] << 18 | Base64Values[temp[1]] << 12;

        reinterpret_cast<uint8_t *>(buffer)[i  ] = n >> 16;
    }
    else if (temp[3] == '=')
    {
        //  This means there's just one padding sextet, so two bytes.

        if ((*len = i + 2) > capacity)
            return DJINN_POLL_PACKET_SIZE_OOR;

        uint32_t const n = Base64Values[temp[0]] << 18 | Base64Values[temp[1]] << 12
                         | Base64Values[temp[2]] <<  6;

        reinterpret_cast<uint8_t *>(buffer)[i++] = n >> 16;
        reinterpret_cast<uint8_t *>(buffer)[i  ] = n >>  8 & 0xFF;
    }
    else
    {
        if (i + 3 > capacity)
        {
            *len = i + 3;

            return DJINN_POLL_PACKET_SIZE_OOR;
        }

        uint32_t const n = Base64Values[temp[0]] << 18 | Base64Values[temp[1]] << 12
                         | Base64Values[temp[2]] <<  6 | Base64Values[temp[3]];

        reinterpret_cast<uint8_t *>(buffer)[i++] = n >> 16;
        reinterpret_cast<uint8_t *>(buffer)[i++] = n >>  8 & 0xFF;
        reinterpret_cast<uint8_t *>(buffer)[i++] = n       & 0xFF;

        goto request_chars;
    }

    //  Reaching this point means padding sextets have been read and the end of
    //  the packet follows.

    while (!DjinnPort->CanRead()) DO_NOTHING();

    Io::In8(DjinnPort->BasePort);   //  Result is discarded for now.

    return DJINN_POLL_SUCCESS;
}
