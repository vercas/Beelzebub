/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


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

#pragma once

#include <stdint.h>

#define COM1	((uint16_t)0x3F8)

/**
 * Sends a byte to an output <port>.
 *
 * @param port port to send the byte to
 * @param value the byte to send
 */
inline __attribute__((always_inline)) void outb(const uint16_t port, uint8_t value)
{
	asm volatile ("outb %1, %0" :: "dN" (port), "a" (value));
}

/**
 * Reads a byte from an input <port>.
 *
 * @param port the port to read from
 * @return the byte read from the port
 */
inline __attribute__((always_inline)) uint8_t inb(const uint16_t port)
{
	uint8_t value;
    asm volatile ("inb %1, %0" : "=a" (value) : "dN" (port));
    return value;
}

void init_serial(const uint16_t port);

char read_serial(const uint16_t port);
void write_serial(const uint16_t port, const char a);

void write_serial_str(const uint16_t port, const char * const a);

void write_serial_ud(const uint16_t port, const uint64_t x);
void write_serial_uh(const uint16_t port, const uint64_t x, const size_t d);
