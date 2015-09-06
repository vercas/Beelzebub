/**
 * THE FOLLOWING LICENSE ONLY APPLIES TO THE FIRST PART OF THE CODE
 *
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

#pragma once

#include <font.h>
#include <stdint.h>

// Screen dimensions
extern uint16_t SCREEN_WIDTH;
extern uint16_t SCREEN_HEIGHT;

// Physical address of the screen's video memory
extern uintptr_t SCREEN_VIDEOMEM_PADDR;

// Video memory as a pointer to screen cells
#define SCREEN_VIDEOMEM         ((screen_cell_t *) SCREEN_VIDEOMEM_PADDR)

// The attribute byte to write to all screen cells
#define SCREEN_ATTRIBUTES       0x0F
#define SCREEN_ATTRIBUTES_PANIC 0x0C

// Writes a message to the screen, then enters an infinite loop.
#define SCREEN_PANIC(msg)       do { \
    puts("\r[XX]\r\nPANIC: " msg); \
    while (true); \
} while (false);

/**
 * A cell in the video memory that contains the displayed character and
 * the respective attributes.
 */
typedef struct screen_cell {
    uint8_t character;
    uint8_t attributes;
} __attribute__((packed)) screen_cell_t;

/**
 * Writes a <message> to the screen, given the coordinates.
 *
 * @param message the message to write.
 * @param x the x coordinate to write the message to.
 * @param y the y coordinate to write the message to.
 */
extern void screen_write(const char *message, uint16_t x, uint16_t y);

/**
 * Writes a <number> to the screen, given the coordinates.
 *
 * @param number the number to write.
 * @param x the x coordinate to write the number to.
 * @param y the y coordinate to write the number to.
 */
extern void screen_write_hex(uint64_t number, uint16_t x, uint16_t y);

/**
 * Clears the screen.
 */
extern void screen_clear(void);



/**
 * HERE THE FIRST PART END.
 *
 * The rest of the code is under the vLicense.
 */

/**
 * Initializes the screen.
 **/
extern void screen_init(void);

/**
 * Writes a string to the screen.
 */
extern size_t puts(const char * const s);

extern size_t puthexs(const uint64_t number);

