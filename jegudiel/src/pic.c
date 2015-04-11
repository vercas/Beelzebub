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

#include <pic.h>
#include <ports.h>
#include <stdint.h>

void pic_setup(void)
{
    // Initialize PIC
    outb(PIC_MASTER_CMD_IO, PIC_ICW1_INIT + PIC_ICW1_ICW4);
    outb(PIC_SLAVE_CMD_IO, PIC_ICW1_INIT + PIC_ICW1_ICW4);
    outb(PIC_MASTER_DATA_IO, PIC_VECTOR);
    outb(PIC_SLAVE_DATA_IO, PIC_VECTOR + 8);
    outb(PIC_MASTER_DATA_IO, 4);
    outb(PIC_SLAVE_DATA_IO, 2);
    outb(PIC_MASTER_DATA_IO, PIC_ICW4_8086);
    outb(PIC_SLAVE_DATA_IO, PIC_ICW4_8086);

    // Mask all IRQs
    outb(PIC_MASTER_DATA_IO, 0xFF);
    outb(PIC_SLAVE_DATA_IO, 0xFF);
}
