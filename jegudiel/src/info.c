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

#include <gdt.h>
#include <jegudiel.h>
#include <idt.h>
#include <info.h>
#include <stdint.h>
#include <string.h>
#include <screen.h>

extern uint8_t info_root_data INFO_SECTION;
extern uint8_t info_mmap_data INFO_SECTION;
extern uint8_t info_module_data INFO_SECTION;
//extern uint8_t info_ioapic_data INFO_SECTION;
extern uint8_t info_strings_data INFO_SECTION;

jg_info_root_t *info_root = (jg_info_root_t *) &info_root_data;
jg_info_mmap_t *info_mmap = (jg_info_mmap_t *) &info_mmap_data;
jg_info_module_t *info_module = (jg_info_module_t *) &info_module_data;

char *info_strings = (char *) &info_strings_data;
char *info_strings_next = (char *) &info_strings_data;
uint32_t info_string_space = 0x1000;

void info_init(void)
{
    info_root->magic = JG_MAGIC;
    info_root->length = 0x5000;
    
    info_root->idt_paddr = (uintptr_t) &idt_data;
    info_root->gdt_paddr = (uintptr_t) &gdt_data;

    info_root->mmap_offset = ((uintptr_t) info_mmap - (uintptr_t) info_root);
    info_root->module_offset = ((uintptr_t) info_module - (uintptr_t) info_root);
    info_root->string_offset = ((uintptr_t) info_strings - (uintptr_t) info_root);
    //info_root->ioapic_offset = ((uintptr_t) info_ioapic - (uintptr_t) info_root);
}

char *info_string_alloc(size_t length)
{
    ++length;

    if (length > info_string_space) {
        SCREEN_PANIC("Ran out of space for strings!");
    }

    char *allocated = info_strings_next;
    info_strings_next = &info_strings_next[length];
    info_string_space -= length;

    return allocated;
}
