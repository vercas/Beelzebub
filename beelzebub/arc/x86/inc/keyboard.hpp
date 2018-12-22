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

#include <system/interrupts.hpp>

#define KEYBOARD_CODE_ESCAPED   0xE0
#define KEYBOARD_CODE_LEFT      0x4B
#define KEYBOARD_CODE_RIGHT     0x4D
#define KEYBOARD_CODE_UP        0x48
#define KEYBOARD_CODE_DOWN      0x50

#define KEYBOARD_IRQ_VECTOR     0xEF

/**
 * Whether the escape scan code has been sent and the next
 * scan code is escaped.
 */
__extern bool keyboard_escaped;

__extern int volatile breakpointEscaped;
__extern int volatile * volatile breakpointEscapedAux;

/**
 * Initializes the keyboard support.
 */
__extern void keyboard_init(void);

/**
 * Sends a command to the keyboard controller.
 *
 * @param cmd the command to send
 */
__extern void keyboard_send_command(uint8_t cmd);

/**
 * ISR for the keyboard IRQ.
 *
 * @param state state of the system
 */
__extern void keyboard_handler(INTERRUPT_HANDLER_ARGS);
