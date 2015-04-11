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

// 64 bit long mode GDT segment descriptors
#define GDT_KERNEL_CODE     0x209800        //< kernel code (0x08)
#define GDT_KERNEL_DATA     0x209200        //< kernel data (0x10)
#define GDT_USER_CODE       0x20F800        //< user code (0x18)
#define GDT_USER_DATA       0x20F200        //< user data (0x20)

/**
 * The length of the GDT in bytes.
 */
#define GDT_LENGTH          0x1000

/**
 * Pointer to the GDT that can be loaded using the LGDT instruction.
 */
typedef struct gdt_pointer {
    uint16_t length;                //< length in bytes - 1
    uint64_t address;               //< virtual address
} __attribute__((packed)) gdt_pointer_t;

/**
 * The data of the 64 bit General Descriptor Table.
 */
extern uint64_t gdt_data[GDT_LENGTH / sizeof(uint64_t)];

/**
 * The pointer to the 64 bit General Descriptor Table.
 */
extern gdt_pointer_t gdt_pointer;
