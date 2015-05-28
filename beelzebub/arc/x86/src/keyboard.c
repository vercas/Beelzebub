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

#include <isr.h>
#include <keyboard.h>
#include <lapic.h>
#include <ports.h>
#include <stdint.h>
#include <ui.h>

bool keyboard_escaped = false;

void keyboard_init(void)
{
    while (inb(0x64) & 0x1) {
        inb(0x60);
    }

    keyboard_send_command(0xF4);
}

void keyboard_send_command(uint8_t cmd)
{
    while (0 != (inb(0x64) & 0x2)) {}
    outb(0x60, cmd);
}

void keyboard_handler(isr_state_t *state)
{
    uint8_t code = inb(0x60);
    outb(0x61, inb(0x61));

    if (KEYBOARD_CODE_ESCAPED == code) {
        keyboard_escaped = true;

    } else if (keyboard_escaped) {
        switch (code) {
        case KEYBOARD_CODE_LEFT:
            ui_switch_left();
            break;

        case KEYBOARD_CODE_RIGHT:
            ui_switch_right();
            break;

        case KEYBOARD_CODE_UP:
            ui_scroll_up();
            break;

        case KEYBOARD_CODE_DOWN:
            ui_scroll_down();
            break;
        }

        keyboard_escaped = false;
    }

    lapic_eoi();
}
