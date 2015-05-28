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

#pragma once
#include <isr.h>
#include <stdint.h>

#define KEYBOARD_CODE_ESCAPED   0xE0
#define KEYBOARD_CODE_LEFT      0x4B
#define KEYBOARD_CODE_RIGHT     0x4D
#define KEYBOARD_CODE_UP        0x48
#define KEYBOARD_CODE_DOWN      0x50

#define KEYBOARD_IRQ_VECTOR     0x40

/**
 * Whether the escape scan code has been sent and the next
 * scan code is escaped.
 */
extern bool keyboard_escaped;

/**
 * Initializes the keyboard support.
 */
void keyboard_init(void);

/**
 * Sends a command to the keyboard controller.
 *
 * @param cmd the command to send
 */
void keyboard_send_command(uint8_t cmd);

/**
 * ISR for the keyboard IRQ.
 *
 * @param state state of the system
 */
void keyboard_handler(isr_state_t *state);
