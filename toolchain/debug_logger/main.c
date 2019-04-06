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

#define _ALADIN
#include "djinn.h"

int SockFD = 0;
char BuffIn[64], BuffOut[64];

int ReadPacket(void); // Returns packet length or negative integer on error.
int Loop(void); // Returns 0 on success, 1 when the socket closed.

int main(int argc, char * * argv)
{
    struct sockaddr_un addr;
    int ret, reportedConnectionFailure = 0;
    char buff[64];

start_over:
    if ((SockFD = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/beel.sock");

    if (connect(SockFD, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        if (!reportedConnectionFailure)
        {
            perror("connect");
            printf("Looking for socket...\n");
            reportedConnectionFailure = 1;
        }

        close(SockFD);
        sleep(1);

        goto start_over;
    }

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
    int len = ReadPacket();

    if (len < 0) return len;

    struct DjinnSimplePacket  const * DSP = (void *)(BuffIn + 1);
    struct DjinnDwordPacket   const * DDP = (void *)(BuffIn + 1);
    struct DjinnGenericPacket const * DGP = (void *)(BuffIn + 1);

    printf("Pack[%2d]:", len);

    for (int i = 0; i < len; ++i)
        printf(" %02X", BuffIn + i + 1);

    printf("\n");
}

int ReadPacket()
{
    int cursor = 0, len, packLen;

    if ((len = recv(SockFD, BuffIn + cursor, 64 - cursor, 0)) < 0)
    {
        perror("recv first part of packet");
        return -1;
    }
    else if (len == 0)
    {
        puts("Disconnected.");
        return -2;
    }

    packLen = BuffIn[0];

    while (cursor < packLen)
        if ((len = recv(SockFD, BuffIn + cursor, packLen - cursor, 0)) < 0)
        {
            perror("recv mid-packet");
            return -3;
        }

    return packLen;
}
