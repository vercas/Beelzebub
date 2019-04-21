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
#include <inttypes.h>

#include "djinn.h"

// #define DUMP_PACKETS

/************
    State
************/

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
        struct DjinnBytePacket    const * DBP;
        struct DjinnHwordPacket   const * DHP;
        struct DjinnWordPacket    const * DWP;
        struct DjinnDwordPacket   const * DDP;
        struct DjinnGenericPacket const * DGP;
    };
};

struct ReadPacket ReadPacketSock(void);
int SendPacketGSock(struct DjinnGenericPacket const * DGP, int len);

struct ReadPacket ReadPacketSockBase64(void);
int SendPacketGSockBase64(struct DjinnGenericPacket const * DGP, int len);

struct ReadPacket (* ReadPacket)(void); // Returns packet or negative length on error.
int (* SendPacketG)(struct DjinnGenericPacket const * DGP, int len);

static inline int SendPacketS(uint16_t type)
{
    struct DjinnSimplePacket DSP = { type };

    return SendPacketG((struct DjinnGenericPacket const *)&DSP, sizeof(DSP));
}

static inline int SendPacketD(uint16_t type, uint64_t payload)
{
    struct DjinnDwordPacket DDP = { type, payload };

    return SendPacketG((struct DjinnGenericPacket const *)&DDP, sizeof(DDP));
}

/*******************
    Initializing
*******************/

int Loop(void); // Returns 0 on success, 1 when the socket closed.

int main(int argc, char * * argv)
{
    int ret, reportedConnectionFailure = 0;

    // if (argc < 2)
    // {
    //     fprintf(stderr, "Expected one argument: socket to connect to.\n");
    //     return 126;
    // }

start_over:
    if (argc >= 2)
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

    ReadPacket = argc >= 3 ? ReadPacketSockBase64 : ReadPacketSock;
    SendPacketG = argc >= 3 ? SendPacketGSockBase64 : SendPacketGSock;

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

/*******************
    Loop-de-doop
*******************/

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

        case DJINN_PACKET_LOG_INT_DEC:
            printf("%" PRId64, rp.DDP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_UDEC:
            printf("%" PRIu64, rp.DDP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_HEX8_L:
            printf("%02" PRIx8, rp.DBP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_HEX8_U:
            printf("%02" PRIX8, rp.DBP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_HEX16_L:
            printf("%04" PRIx16, rp.DHP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_HEX16_U:
            printf("%04" PRIX16, rp.DHP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_HEX24_L:
            {
                uint32_t val = 0;
                memcpy(&val, rp.DGP->Payload, 3);
                printf("%06" PRIx32, val);
                fflush(stdout);
                break;
            }

        case DJINN_PACKET_LOG_INT_HEX24_U:
            {
                uint32_t val = 0;
                memcpy(&val, rp.DGP->Payload, 3);
                printf("%06" PRIX32, val);
                fflush(stdout);
                break;
            }

        case DJINN_PACKET_LOG_INT_HEX32_L:
            printf("%08" PRIx32, rp.DWP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_HEX32_U:
            printf("%08" PRIX32, rp.DWP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_HEX48_L:
            {
                uint64_t val = 0;
                memcpy(&val, rp.DGP->Payload, 6);
                printf("%012" PRIx64, val);
                fflush(stdout);
                break;
            }

        case DJINN_PACKET_LOG_INT_HEX48_U:
            {
                uint64_t val = 0;
                memcpy(&val, rp.DGP->Payload, 6);
                printf("%012" PRIX64, val);
                fflush(stdout);
                break;
            }

        case DJINN_PACKET_LOG_INT_HEX64_L:
            printf("%016" PRIx64, rp.DDP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_HEX64_U:
            printf("%016" PRIX64, rp.DDP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_HEX_VAR_L:
            printf("%" PRIx64, rp.DDP->Payload);
            fflush(stdout);
            break;

        case DJINN_PACKET_LOG_INT_HEX_VAR_U:
            printf("%" PRIX64, rp.DDP->Payload);
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

/**************
    Sockets
**************/

struct ReadPacket ReadPacketSock()
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

int SendPacketGSock(struct DjinnGenericPacket const * DGP, int len)
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

/******************************
    Sockets, base64-encoded
******************************/

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

struct ReadPacket ReadPacketSockBase64()
{
    static uint8_t BuffIn[4096], BuffPack[3072];
    static int BuffLen = 0, LastPackEnd = 0;

    int cursor, len;
    struct ReadPacket ret = { 0, { (struct DjinnSimplePacket *)BuffPack } };

    if (BuffLen > LastPackEnd)
    {
        //  If there is data in the buffer, remove a packet from it.

        cursor = BuffLen - LastPackEnd;
#ifdef DUMP_PACKETS
        printf("Shifting BuffIn: %p [%d] <- %p | %d\n", BuffIn, BuffLen, BuffIn + LastPackEnd, cursor);
#endif
        memmove(BuffIn, BuffIn + LastPackEnd, cursor);

#ifdef DUMP_PACKETS
        printf("Shifted!\n");
#endif
    }
    else
        cursor = 0;

    LastPackEnd = 0;
    size_t i = 0;

    do
    {
        for (/* nothing */; i < cursor; ++i)
        {
            switch (BuffIn[i])
            {
            case '\n':  //  End of message.
// #ifdef DUMP_PACKETS
//                 printf("Newline at %d\n", i);
// #endif

                LastPackEnd = i + 1;
                goto post_rec_loop;

            case '\r':  //  AFAIK these are strays...
#ifdef DUMP_PACKETS
                printf("Carriage return at %zd\n", i);
#endif
                if (cursor > i + 1)
                    memmove(BuffIn + i, BuffIn + i + 1, cursor - i - 1);

                --i, --cursor;
                break;

// #ifdef DUMP_PACKETS
//             default:
//                 printf("'%c' at %zd\n", BuffIn[i], i);
//                 break;
// #endif
            }
        }

        if (cursor >= sizeof(BuffIn))
        {
            puts("Input buffer overflow.");
            ret.Length = -2;
            return ret;
        }

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
            cursor += len;
    }
    while (LastPackEnd == 0);

post_rec_loop:
    BuffLen = cursor;

    if (LastPackEnd % 4 != 1)
        printf("Unaligned packet? %d %d\n", LastPackEnd, LastPackEnd % 4);

    size_t j = 0;

    for (i = 0; i < LastPackEnd - 5; i += 4)
    {
        uint32_t const n = Base64Values[BuffIn[i    ]] << 18 | Base64Values[BuffIn[i + 1]] << 12
                         | Base64Values[BuffIn[i + 2]] <<  6 | Base64Values[BuffIn[i + 3]];
        BuffPack[j++] = n >> 16;
        BuffPack[j++] = n >> 8 & 0xFF;
        BuffPack[j++] = n & 0xFF;
    }

    if (BuffIn[i + 2] == '=')
    {
        //  This means there's two padding sextets, so just one byte.

        uint32_t const n = Base64Values[BuffIn[i + 0]] << 18 | Base64Values[BuffIn[i + 1]] << 12;

        BuffPack[j++] = n >> 16;
    }
    else if (BuffIn[i + 3] == '=')
    {
        //  This means there's just one padding sextet, so two bytes.

        uint32_t const n = Base64Values[BuffIn[i + 0]] << 18 | Base64Values[BuffIn[i + 1]] << 12
                         | Base64Values[BuffIn[i + 2]] <<  6;

        BuffPack[j++] = n >> 16;
        BuffPack[j++] = n >>  8 & 0xFF;
    }
    else
    {
        uint32_t const n = Base64Values[BuffIn[i + 0]] << 18 | Base64Values[BuffIn[i + 1]] << 12
                         | Base64Values[BuffIn[i + 2]] <<  6 | Base64Values[BuffIn[i + 3]];

        BuffPack[j++] = n >> 16;
        BuffPack[j++] = n >>  8 & 0xFF;
        BuffPack[j++] = n       & 0xFF;
    }

#ifdef DUMP_PACKETS
    printf(" IN [%2d]:", j);

    for (i = 0; i < j; ++i)
        printf(" %02X", (unsigned int)BuffPack[i] & 0xFF);

    printf("\n");

    fwrite(BuffIn, LastPackEnd, 1, stdout);
    fflush(stdout);
#endif

    ret.Length = j;
    return ret;
}

static uint8_t const Base64Chars[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int SendPacketGSockBase64(struct DjinnGenericPacket const * DGP, int len)
{
    static uint8_t BuffOut[1536];

    size_t oLen = 4 * ((len + 2) / 3);

    if (oLen >= 1536)
        return -10;

    uint8_t * pos = BuffOut;
    uint8_t const * in = (uint8_t const *)DGP;
    uint8_t const * end = in + len;

    for (/* nothing */; end - in >= 3; in += 3)
    {
        *pos++ = Base64Chars[                         in[0] >> 2 ];
        *pos++ = Base64Chars[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = Base64Chars[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = Base64Chars[  in[2] & 0x3f                      ];
    }

    if (end - in)
    {
        *pos++ = Base64Chars[in[0] >> 2];

        if (end - in == 1)
        {
            *pos++ = Base64Chars[ (in[0] & 0x03) << 4                ];
            *pos++ = '=';
        }
        else
        {
            *pos++ = Base64Chars[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = Base64Chars[ (in[1] & 0x0f) << 2                ];
        }

        *pos++ = '=';
    }

    *pos++ = '\n';
    *pos = '\0';

#ifdef DUMP_PACKETS
    printf("OUT [%2d]:", len);

    for (int i = 0; i < len; ++i)
        printf(" %02X", (unsigned int)*((uint8_t const *)DGP + i) & 0xFF);

    printf("\n");

    puts((char const *)BuffOut);
#endif

    int cursor = 0, ret;

    while (BuffOut + cursor < pos)
        if ((ret = send(SockFD, BuffOut + cursor, pos - BuffOut - cursor, 0)) < 0)
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
