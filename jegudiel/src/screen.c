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

void screen_write(const char *message, uint16_t x, uint16_t y)
{
    size_t i;
    size_t position = x + y * SCREEN_WIDTH;

    for (i = 0; '\0' != message[i]; ++i) {
        if (y == 24) SCREEN_VIDEOMEM[position].attributes = SCREEN_ATTRIBUTES_PANIC;
        SCREEN_VIDEOMEM[position++].character = message[i];
    }
}

void screen_write_hex(uint64_t number, uint16_t x, uint16_t y)
{
    char string[17];
    string[16] = '\0';

    size_t i;
    for (i = 0; i < 16; ++i) {
        uint8_t nibble = (number >> (i * 4)) & 0xF;

        if (nibble >= 10) {
            string[15 - i] = 'A' + (nibble - 10);
        } else {
            string[15 - i] = '0' + nibble;
        }
    }

    screen_write(string, x, y);
}

/**
 * Clears the screen.
 */
void screen_clear(void)
{
    size_t i;
    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        screen_cell_t *cell = &SCREEN_VIDEOMEM[i];
        cell->attributes = SCREEN_ATTRIBUTES;
        cell->character = ' ';
    }
}
