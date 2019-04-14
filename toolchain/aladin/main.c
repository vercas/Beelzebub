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

#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#define _ALADIN
#include "djinn.h"

//#define DUMP_PACKETS

enum STATE
{
    AwaitingHandshake,
    AwaitingHandshakeResponse,
    Logging
} State;

int SockFD = 0;

struct ReadPacket
{
    int Length;

    union
    {
        struct DjinnSimplePacket  const * DSP;
        struct DjinnDwordPacket   const * DDP;
        struct DjinnGenericPacket const * DGP;
    };
};

struct ReadPacket ReadPacket(void); // Returns packet or negative length on error.
int Loop(void); // Returns 0 on success, 1 when the socket closed.

int SendPacketS(uint16_t type);
int SendPacketD(uint16_t type, uint64_t payload);
int SendPacketG(struct DjinnGenericPacket const * DGP, int len);

int main(int argc, char * * argv)
{
    struct sockaddr_un addr;
    int ret, reportedConnectionFailure = 0;

    if (argc < 2)
    {
        fprintf(stderr, "Expected one argument: socket to connect to.\n");
        return 126;
    }

start_over:
    if ((SockFD = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, argv[1], sizeof(addr.sun_path)-1);

    if (connect(SockFD, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        if (reportedConnectionFailure != errno)
        {
            perror("connect");
            printf("Looking for socket %s...\n", addr.sun_path);
            reportedConnectionFailure = errno;
        }

        close(SockFD);
        sleep(1);

        goto start_over;
    }

    printf("Connected.\n");
    State = AwaitingHandshake;

    ret = Loop();

    close(SockFD);

    if (ret)
    {
        reportedConnectionFailure = 0;

        goto start_over;
    }

    return 0;
}

int Loop(void)
{
    struct ReadPacket rp;

loop_again:
    if ((rp = ReadPacket()).Length < 0)
        return rp.Length;

    switch (State)
    {
    case AwaitingHandshake:
        if (rp.DDP->Type != DJINN_PACKET_HANDSHAKE0)
        {
            printf("Expected DJINN_PACKET_HANDSHAKE0 (%u), got %u instead.\n", DJINN_PACKET_HANDSHAKE0, rp.DDP->Type);
            return -100;
        }
        else
        {
            if (rp.DDP->Payload != 0xF00000FF00FFF000ULL)
            {
                printf("Expected DJINN_PACKET_HANDSHAKE0 payload to be %lluX, got %lluX instead.\n"
                    , 0xF00000FF00FFF000ULL, rp.DDP->Payload);
                return -101;
            }

            SendPacketD(DJINN_PACKET_HANDSHAKE1, 0x123456789ABCDEF0);
            State = AwaitingHandshakeResponse;
            break;
        }

    case AwaitingHandshakeResponse:
        if (rp.DDP->Type != DJINN_PACKET_HANDSHAKE2)
        {
            printf("Expected DJINN_PACKET_HANDSHAKE2 (%u), got %u instead.\n", DJINN_PACKET_HANDSHAKE2, rp.DDP->Type);
            return -110;
        }
        else
        {
            if (rp.DDP->Payload != (0x123456789ABCDEF0 ^ 0xF00000FF00FFF000ULL))
            {
                printf("Expected DJINN_PACKET_HANDSHAKE2 payload to be %lluX, got %lluX instead.\n"
                    , 0x123456789ABCDEF0 ^ 0xF00000FF00FFF000ULL, rp.DDP->Payload);
                return -111;
            }

            State = Logging;
            break;
        }

    case Logging:
        switch (rp.DSP->Type)
        {
        case DJINN_PACKET_LOG:
            //printf("%.*s", len - 3, rp.DGP->Payload);
            fwrite(rp.DGP->Payload, rp.Length - sizeof(rp.DGP->Type), 1, stdout);
            fflush(stdout);
            break;

        default:
            printf("Unsupport packet type: %u\n", rp.DSP->Type);
            break;
        }

        break;

    default:
        printf("Unknown state!\n");
        return -1000;
    }

    goto loop_again;
}

struct ReadPacket ReadPacket()
{
    static char BuffIn[4096];
    static int BuffLen = 0;

    int cursor, len, packLen;
    struct ReadPacket ret = { 0, { (struct DjinnSimplePacket *)(BuffIn + 1) } };

    if (BuffLen > 0)
    {
        //  If there is data in the buffer, remove a packet from it.

        cursor = BuffLen - BuffIn[0] - 1;
#ifdef DUMP_PACKETS
        printf("Shifting BuffIn: %p [%d] <- %p | %d\n", BuffIn, BuffLen, BuffIn + BuffIn[0] + 1, cursor);
#endif
        if (cursor > 0)
            memmove(BuffIn, BuffIn + BuffIn[0] + 1, cursor);
    }
    else
        cursor = 0;

    if (cursor == 0)
        packLen = sizeof(BuffIn);   //  No length available (yet) means read as much as possible.
    else
        packLen = BuffIn[0] + 1;    //  Account for the size prefix.

    while (cursor < packLen)
        if ((len = recv(SockFD, BuffIn + cursor, sizeof(BuffIn) - cursor, 0)) < 0)
        {
            perror("recv packet");
            ret.Length = -1;
            return ret;
        }
        else if (len == 0)
        {
            puts("Disconnected.");
            ret.Length = -2;
            return ret;
        }
        else
        {
            if (cursor == 0)
                packLen = BuffIn[0] + 1;    //  Now the size is known!

            cursor += len;
        }

#ifdef DUMP_PACKETS
    printf(" IN [%2d]:", packLen - 1);

    for (int i = 1; i < packLen; ++i)
        printf(" %02X", (unsigned int)BuffIn[i] & 0xFF);

    printf("\n");
#endif

    BuffLen = cursor;

    ret.Length = packLen - 1;
    return ret;
}

int SendPacketS(uint16_t type)
{
    struct DjinnSimplePacket DSP = { type };

    return SendPacketG((struct DjinnGenericPacket const *)&DSP, sizeof(DSP));
}

int SendPacketD(uint16_t type, uint64_t payload)
{
    struct DjinnDwordPacket DDP = { type, payload };

    return SendPacketG((struct DjinnGenericPacket const *)&DDP, sizeof(DDP));
}

int SendPacketG(struct DjinnGenericPacket const * DGP, int len)
{
    int cursor = 0, ret;
    static char BuffOut[256];
    BuffOut[0] = len;
    memcpy(BuffOut + 1, DGP, len++);

#ifdef DUMP_PACKETS
    printf("OUT [%2d]:", BuffOut[0]);

    for (int i = 1; i < len; ++i)
        printf(" %02X", (unsigned int)BuffOut[i] & 0xFF);

    printf("\n");
#endif

    while (cursor < len)
        if ((ret = send(SockFD, BuffOut + cursor, len - cursor, 0)) < 0)
        {
            perror("send packet");
            return -1;
        }
        else if (ret == 0)
        {
            puts("Disconnected.");
            return -2;
        }
        else
            cursor += ret;

    return cursor;
}
