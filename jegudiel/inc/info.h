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
#include "jegudiel.h"

/**
 * Macro to link a data structure into the info section.
 */
#define INFO_SECTION __attribute__((section (".info")))

/**
 * Pointer to the root structure of the info section.
 */
extern jg_info_root_t *info_root;

/**
 * Pointer to the CPU list of the info section.
 */
extern jg_info_cpu_t *info_cpu;

/**
 * Pointer to the IO APIC list of the info section.
 */
extern jg_info_ioapic_t *info_ioapic;

/**
 * Pointer to the memory map list of the info section.
 */
extern jg_info_mmap_t *info_mmap;

/**
 * Pointer to the module list of the info section.
 */
extern jg_info_module_t *info_module;

/**
 * Pointer to the string table of the info section.
 */
extern char *info_strings;

/**
 * Pointer to the place to put the next string to.
 */
extern char *info_strings_next;

/**
 * Space that is left in the string table (in bytes).
 */
extern uint32_t info_string_space;

// Linker symbols that specify the beginning and
// the end of the info tables in memory
extern uint8_t info_begin;
extern uint8_t info_end;

/**
 * Prepares the info structure by filling it with reasonable defaults.
 */
void info_init(void);

/**
 * Copies the tables to the target location and compacts them to the given size.
 * 
 * Panics, if the size does not suffice for storing the tables.
 * 
 * @param target
 */
void info_copy(void *target, size_t size);

/**
 * Allocates space for a string, given its length.
 * 
 * The given length does not include the terminating zero byte.
 * 
 * Panics, when running out of space in the string table.
 * 
 * @param length the length of the string to allocate.
 * @return pointer to the allocate string
 */
char *info_string_alloc(size_t length);
