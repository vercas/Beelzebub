/**
 * Copyright (c) 2012 by Lukas Heidemann <lukasheidemann@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <screen.h>
#include <stdint.h>

void screen_write(const char *string, uint16_t x, uint16_t y)
{
    char *video = (char *) SCREEN_MEMORY;
    size_t screen_max = SCREEN_WIDTH * SCREEN_HEIGHT;

    size_t pos_screen = x + y * SCREEN_WIDTH;
    size_t pos_str;

    for (pos_str = 0; '\0' != string[pos_str] && pos_screen < screen_max; ++pos_str) {
        if ('\n' == string[pos_str]) {
            pos_screen = ((pos_screen / SCREEN_WIDTH) + 1) * SCREEN_WIDTH;
        } else {
            video[pos_screen * 2    ] = string[pos_str];
            video[pos_screen * 2 + 1] = SCREEN_ATTR;
            ++pos_screen;
        }
    }
}

void screen_clear(void)
{
    char *video = (char *) SCREEN_MEMORY;
    size_t screen_max = SCREEN_WIDTH * SCREEN_HEIGHT;

    size_t i;
    for (i = 0; i < screen_max; ++i) {
        video[i * 2    ] = ' ';
        video[i * 2 + 1] = SCREEN_ATTR;
    }
}
