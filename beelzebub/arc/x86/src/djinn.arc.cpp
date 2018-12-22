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
using namespace Beelzebub::System;

ManagedSerialPort * DjinnPort = nullptr;

__hot DJINN_SEND_RES DjinnSendPacketThroughSerialPort(void const * packet, size_t size)
{
    if (size > 0xFF)
        return DJINN_SEND_PACKET_SIZE_OOR;

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

__hot DJINN_POLL_RES DjinnPollPacketFromSerialPort(void * buffer, size_t capacity, size_t * len)
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

Handle Beelzebub::InitializeDebuggerInterface(DjinnInterfaces iface)
{
    switch (iface)
    {
    case DjinnInterfaces::COM1: DjinnPort = &COM1; break;
    case DjinnInterfaces::COM2: DjinnPort = &COM2; break;
    case DjinnInterfaces::COM3: DjinnPort = &COM3; break;
    case DjinnInterfaces::COM4: DjinnPort = &COM4; break;

    default:
        return HandleResult::NotImplemented;
    }

    DjinnInitData id {};

    if (DjinnPort != nullptr)
    {
        id.Sender = &DjinnSendPacketThroughSerialPort;
        id.Poller = &DjinnPollPacketFromSerialPort;
        id.PacketMaxSize = DjinnPort->QueueSize - 1;
    }
    else
        return HandleResult::Failed;

    DJINN_INIT_RES res = DjinnInitialize(&id);

    if (res != DJINN_INIT_SUCCESS)
        return HandleResult::Failed;

    return HandleResult::Okay;
}
