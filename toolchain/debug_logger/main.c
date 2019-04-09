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
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#define _ALADIN
#include "djinn.h"

enum STATE
{
    AwaitingHandshake,
    AwaitingHandshakeResponse,
    Logging
} State;

int SockFD = 0, awaitingBogusByte;
char BuffIn[256];

int ReadPacket(void); // Returns packet length or negative integer on error.
int Loop(void); // Returns 0 on success, 1 when the socket closed.

int SendPacketS(uint16_t type);
int SendPacketD(uint16_t type, uint64_t payload);
int SendPacketG(struct DjinnGenericPacket const * DGP, int len);

int main(int argc, char * * argv)
{
    int ret, reportedConnectionFailure = 0;

    // if (argc < 2)
    // {
    //     fprintf(stderr, "Expected one argument: socket to connect to.\n");
    //     return 126;
    // }

start_over:
    if (argc == 2)
    {
        struct sockaddr_un addr;

        if ((SockFD = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        {
            perror("Unix socket");
            return 1;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, argv[1], sizeof(addr.sun_path)-1);

        if (connect(SockFD, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            if (reportedConnectionFailure != errno)
            {
                perror("Unix socket connect");
                printf("Looking for Unix socket %s...\n", addr.sun_path);
                reportedConnectionFailure = errno;
            }

            close(SockFD);
            sleep(1);

            goto start_over;
        }
    }
    else
    {
        struct sockaddr_in addr;

        if ((SockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("TCP socket");
            return 1;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
        addr.sin_port = htons(2345);

        if (connect(SockFD, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            if (reportedConnectionFailure != errno)
            {
                perror("TCP socket connect");
                printf("Looking for TCP server 127.0.0.1:2345...\n");
                reportedConnectionFailure = errno;
            }

            close(SockFD);
            sleep(1);

            goto start_over;
        }
    }

    printf("Connected.\n");
    awaitingBogusByte = 1;
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
    int len;

loop_again:
    if ((len = ReadPacket()) < 0)
        return len;

    struct DjinnSimplePacket  const * DSP = (void *)(BuffIn + 1);
    struct DjinnDwordPacket   const * DDP = (void *)(BuffIn + 1);
    struct DjinnGenericPacket const * DGP = (void *)(BuffIn + 1);

    printf("Pack[%2d]:", len);

    for (int i = 0; i < len; ++i)
        printf(" %02X", (unsigned int)BuffIn[i + 1] & 0xFF);

    printf("\n");

    switch (State)
    {
    case AwaitingHandshake:
        if (DDP->Type != DJINN_PACKET_HANDSHAKE0)
        {
            printf("Expected DJINN_PACKET_HANDSHAKE0 (%u), got %u instead.\n", DJINN_PACKET_HANDSHAKE0, DDP->Type);
            return -100;
        }
        else
        {
            if (DDP->Payload != 0xF00000FF00FFF000ULL)
            {
                printf("Expected DJINN_PACKET_HANDSHAKE0 payload to be %ullX, got %ullX instead.\n"
                    , 0xF00000FF00FFF000ULL, DDP->Payload);
                return -101;
            }

            SendPacketD(DJINN_PACKET_HANDSHAKE1, 0x123456789ABCDEF0);
            State = AwaitingHandshakeResponse;
            break;
        }

    case AwaitingHandshakeResponse:
        if (DDP->Type != DJINN_PACKET_HANDSHAKE2)
        {
            printf("Expected DJINN_PACKET_HANDSHAKE2 (%u), got %u instead.\n", DJINN_PACKET_HANDSHAKE2, DDP->Type);
            return -110;
        }
        else
        {
            if (DDP->Payload != (0x123456789ABCDEF0 ^ 0xF00000FF00FFF000ULL))
            {
                printf("Expected DJINN_PACKET_HANDSHAKE2 payload to be %ullX, got %ullX instead.\n"
                    , 0x123456789ABCDEF0 ^ 0xF00000FF00FFF000ULL, DDP->Payload);
                return -111;
            }

            State = Logging;
            break;
        }

    case Logging:
        switch (DSP->Type)
        {
        case DJINN_PACKET_LOG:
            printf("%.*s", len - 3, DGP->Payload);
            break;

        default:
            printf("Unsupport packet type: %u\n", DSP->Type);
        }

    default:
        printf("Unknown state!\n");
        return -1000;
    }

    goto loop_again;
}

int ReadPacket()
{
    int cursor, len, packLen;

    // printf("Reading new packet.\n");

    if ((cursor = recv(SockFD, BuffIn, sizeof(BuffIn), 0)) < 0)
    {
        perror("recv first part of packet");
        return -1;
    }
    else if (cursor == 0)
    {
        puts("Disconnected.");
        return -2;
    }

    packLen = BuffIn[0];

    if (awaitingBogusByte && packLen == '\r')
    {
        printf("Discarding bizzare carriage return at the beginning of the package.\n");

        memmove(BuffIn, BuffIn + 1, --cursor);
        packLen = BuffIn[0];

        awaitingBogusByte = 0;
    }

    ++packLen;  //  Account for the size prefix.

    // for (int i = 0; i < cursor; ++i)
        // printf(" %02uX", BuffIn[i]);

    // printf("\n");

    // printf("Read %d bytes at first, length is %d.\n", cursor, packLen);

    while (cursor < packLen)
        if ((len = recv(SockFD, BuffIn + cursor, packLen - cursor, 0)) < 0)
        {
            perror("recv mid-packet");
            return -3;
        }
        else if (len == 0)
        {
            puts("Disconnected.");
            return -2;
        }
        else
        {
            // for (int i = 0; i < len; ++i)
                // printf(" %02uX", BuffIn[cursor + i]);

            // printf("\n");

            cursor += len;
            // printf("Read %d more bytes for a total of %d bytes.\n", len, cursor);
        }

    // printf("Packet done.\n");

    return packLen - 1; //  Accounting again for size prefix.
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

    while (cursor < len)
        if ((ret = send(SockFD, (char *)DGP + cursor, len - cursor, 0)) < 0)
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
