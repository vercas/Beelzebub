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
#include <stdint.h>

// IO ports for the master PIC
#define PIC_MASTER_IO       0x20
#define PIC_MASTER_CMD_IO   PIC_MASTER_IO
#define PIC_MASTER_DATA_IO  (PIC_MASTER_IO + 1)

// IO ports for the slave PIC
#define PIC_SLAVE_IO        0xA0
#define PIC_SLAVE_CMD_IO    PIC_SLAVE_IO
#define PIC_SLAVE_DATA_IO   (PIC_SLAVE_IO + 1)

// ICW1
#define PIC_ICW1_ICW4       0x01        //< ICW4 needed
#define PIC_ICW1_SINGLE     0x02        //< Single mode (default: cascade)
#define PIC_ICW1_INTERVAL4  0x04        //< Call address interval 4 (default: 8)
#define PIC_ICW1_LEVEL      0x08        //< Level triggered mode (default: edge)
#define PIC_ICW1_INIT       0x10        //< Initialization

// ICW4
#define PIC_ICW4_8086       0x01        // 8086/88 mode (default: MCS-80/85)
#define PIC_ICW4_AUTO       0x02        // Auto EOI (default: normal)
#define PIC_ICW4_BUF_SLAVE  0x08        // Buffered mode/slave
#define PIC_ICW4_BUF_MASTER 0x0C        // Buffered mode/master
#define PIC_ICW4_SFNM       0x10        // Special fully nested

// The PIC base vector
#define PIC_VECTOR          0x70

/**
 * Initializes the PIC to a well defined state, then masks all IRQs.
 */
void pic_setup(void);
