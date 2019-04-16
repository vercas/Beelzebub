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

#include <djinn.h>
#include "packets.hpp"
#include <string.h>

using namespace Djinn;

//  Initialization

enum DJINN_INIT_STATUS
{
    Uninitialized, InitInProgress, Initialized,
};

::DjinnInitData InitData;
DJINN_INIT_STATUS InitStatus = Uninitialized;
int DebuggerCount = 0;

template<typename TPacket, bool BRetry = true>
static DJINN_SEND_RES Send(TPacket const pack)
{
    DJINN_SEND_RES res;

retry:
    res = InitData.Sender(&pack, sizeof(pack));
    
    if (BRetry && res == DJINN_SEND_AWAIT)
    {
        DJINN_DO_NOTHING();
        goto retry;
    }

    return res;
}

template<typename TPacket, bool BRetry = true>
static DJINN_POLL_RES Poll(TPacket & pack, size_t & len, uint64_t timeout = 0)
{
    DJINN_POLL_RES res;

retry:
    res = InitData.Poller(&pack, sizeof(pack), &len);
    
    if (BRetry && res == DJINN_POLL_AWAIT)
    {
        DJINN_DO_NOTHING();
        goto retry;
    }
    else if (res == DJINN_POLL_NOTHING && timeout > 0)
    {
        --timeout;
        DJINN_DO_NOTHING();
        //  TODO: Some form of proper timing here, e.g. based on the TSC.

        goto retry;
    }

    return res;
}

template<typename TPacket, bool BRetry = true>
static DJINN_POLL_RES Poll(TPacket & pack, uint64_t timeout = 0)
{
    size_t len;
    DJINN_POLL_RES res = Poll(pack, len, timeout);

    if (len != sizeof(TPacket))
        return DJINN_POLL_PACKET_SIZE_OOR;

    return res;
}

DJINN_INIT_RES DjinnInitialize(DjinnInitData * data)
{
    DJINN_INIT_STATUS initStatus = Uninitialized;

    if (!__atomic_compare_exchange_n(&InitStatus, &initStatus, InitInProgress, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
        return DJINN_INIT_TOO_LATE;
#define FAIL(val) do { __atomic_store_n(&InitStatus, Uninitialized, __ATOMIC_RELEASE); return (val); } while(false);

    InitData = *data;

    if (InitData.Sender == nullptr || InitData.Poller == nullptr)
        FAIL(DJINN_INIT_DATA_INVALID);

    if (InitData.PacketMaxSize < 6)
        FAIL(DJINN_INIT_PACKET_TOO_SMALL);

    //  Data seems okay. Obtain nuance.

    uint64_t const nuance = 0xF00000FF00FFF000ULL;

    //  Send first handshake packet.

    for (size_t volatile i = 0; i < 1000000000; ++i) { }

    DJINN_SEND_RES sres = Send(DwordPacket(HandshakePacket0, nuance));

    if (sres != DJINN_SEND_SUCCESS)
        FAIL(DJINN_INIT_HANDSHAKE_FAILED);

    DwordPacket buf1;
    DJINN_POLL_RES pres = Poll(buf1, 1000000);
    //  TODO: Timing.

    if (pres == DJINN_POLL_NOTHING)
        goto success;
    //  No response simply means the debugger is not listening.

    if (pres != DJINN_POLL_SUCCESS || buf1.Type != HandshakePacket1)
        FAIL(DJINN_INIT_HANDSHAKE_FAILED);

    sres = Send(DwordPacket(HandshakePacket2, buf1.Payload ^ nuance));

    if (sres != DJINN_SEND_SUCCESS)
        FAIL(DJINN_INIT_HANDSHAKE_FAILED);

    ++DebuggerCount;

#undef FAIL
success:
    __atomic_store_n(&InitStatus, Uninitialized, __ATOMIC_RELEASE);
    return DJINN_INIT_SUCCESS;
}

//  Status

bool DjinnGetInitialized()
{
    return __atomic_load_n(&InitStatus, __ATOMIC_RELAXED) == Initialized;
}

int DjinnGetDebuggerCount()
{
    return __atomic_load_n(&DebuggerCount, __ATOMIC_RELAXED);
}

//  Logging

DjinnLogResult DjinnLog(char const * str, int cnt)
{
    if (__atomic_load_n(&DebuggerCount, __ATOMIC_ACQUIRE) == 0)
        return { DJINN_LOG_NO_DEBUGGERS, 0 };

    if (cnt > (int)(InitData.PacketMaxSize - sizeof(SimplePacket)))
        cnt = (int)(InitData.PacketMaxSize - sizeof(SimplePacket));

    if (reinterpret_cast<uintptr_t>(str) < 0xFFFF000000000000ULL)
    {
        str = "DAFUQ?!";
        cnt = 7;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
    size_t const pSize = cnt + sizeof(SimplePacket);
    uint8_t buf[pSize];
    uint8_t * const bufPtr = &(buf[0]);
#pragma GCC diagnostic pop

    new (reinterpret_cast<SimplePacket *>(bufPtr)) SimplePacket(LogPacket);
    memcpy(bufPtr + sizeof(SimplePacket), str, cnt);

    DJINN_SEND_RES res;

retry:
    res = InitData.Sender(bufPtr, pSize);

    if (res == DJINN_SEND_AWAIT)
    {
        DJINN_DO_NOTHING();
        goto retry;
    }

    if (res == DJINN_SEND_SUCCESS)
        return { DJINN_LOG_SUCCESS, cnt };
    else
        return { DJINN_LOG_FAIL, 0 };
}

DjinnLogResult DjinnLogUInt(uint64_t val, DJINN_LOG_INT_FORMAT fmt)
{
    if (__atomic_load_n(&DebuggerCount, __ATOMIC_ACQUIRE) == 0)
        return { DJINN_LOG_NO_DEBUGGERS, 0 };

    DwordPacket out(LogIntHexVarUPacket, val);
    //  Works with little endian numbers.

    DJINN_SEND_RES res;
    size_t pSize = sizeof(SimplePacket);

    switch (fmt)
    {
    case DJINN_INT_DEC:       out.Type = LogIntDecPacket;     pSize += 8;  break;
    case DJINN_INT_UDEC:      out.Type = LogIntUDecPacket;    pSize += 8;  break;
    case DJINN_INT_HEX8_L:    out.Type = LogIntHex8LPacket;   pSize += 1;  break;
    case DJINN_INT_HEX8_U:    out.Type = LogIntHex8UPacket;   pSize += 1;  break;
    case DJINN_INT_HEX16_L:   out.Type = LogIntHex16LPacket;  pSize += 2;  break;
    case DJINN_INT_HEX16_U:   out.Type = LogIntHex16UPacket;  pSize += 2;  break;
    case DJINN_INT_HEX24_L:   out.Type = LogIntHex24LPacket;  pSize += 3;  break;
    case DJINN_INT_HEX24_U:   out.Type = LogIntHex24UPacket;  pSize += 3;  break;
    case DJINN_INT_HEX32_L:   out.Type = LogIntHex32LPacket;  pSize += 4;  break;
    case DJINN_INT_HEX32_U:   out.Type = LogIntHex32UPacket;  pSize += 4;  break;
    case DJINN_INT_HEX48_L:   out.Type = LogIntHex48LPacket;  pSize += 6;  break;
    case DJINN_INT_HEX48_U:   out.Type = LogIntHex48UPacket;  pSize += 6;  break;
    case DJINN_INT_HEX64_L:   out.Type = LogIntHex64LPacket;  pSize += 8;  break;
    case DJINN_INT_HEX64_U:   out.Type = LogIntHex64UPacket;  pSize += 8;  break;
    case DJINN_INT_HEX_VAR_L: out.Type = LogIntHexVarLPacket; pSize += 8;  break;
    case DJINN_INT_HEX_VAR_U: out.Type = LogIntHexVarUPacket; pSize += 8;  break;
    }

    if (pSize > InitData.PacketMaxSize)
        return { DJINN_LOG_STRINGIFY, 0 };
    //  This means the caller better provide a string because the number is too
    //  large for the underlying transport protocol.

retry:
    res = InitData.Sender(&out, pSize);

    if (res == DJINN_SEND_AWAIT)
    {
        DJINN_DO_NOTHING();
        goto retry;
    }

    if (res == DJINN_SEND_SUCCESS)
        return { DJINN_LOG_SUCCESS, (int)(pSize - sizeof(SimplePacket)) };
    else
        return { DJINN_LOG_FAIL, 0 };
}
