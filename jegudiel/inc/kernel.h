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
#include <jegudiel.h>

/**
 * String the module name of the kernel binary module must begin with.
 */
#define KERNEL_NAME "kernel64"

/**
 * Pointer to the kernel binary, after it has been discovered by kernel_find().
 */
extern void *kernel_binary;

/**
 * Pointer to the kernel header, after it has been discovered by kernel_analyze().
 */
extern jg_header_root_t *kernel_header;

/**
 * Finds the kernel binary module or panics if there is none.
 */
void kernel_find(void);

/**
 * Checks the kernel binary and panics if its invalid.
 */
void kernel_check(void);

/**
 * Locates the kernel header in the kernel binary or panics if there is none.
 */
void kernel_analyze(void);

/**
 * Maps the stack of the current CPU to the virtual address specified in the
 * kernel header, if any. Also moves the stack pointer to an equivalent
 * offset in the mapped stack.
 */
void kernel_map_stack(void);

/**
 * Maps the info tables to the virtual address specified in the kernel header,
 * if any.
 */
void kernel_map_info(void);

/**
 * Maps and reloads the IDT.
 */
void kernel_map_idt(void);

/**
 * Maps and reloads the GDT.
 */
void kernel_map_gdt(void);

/**
 * Jumps to the kernel's BSP entry point
 */
void kernel_enter_bsp(void);

/**
 * Jumps to the kernel's AP entry point or halts, if there is none.
 */
void kernel_enter_ap(void);
