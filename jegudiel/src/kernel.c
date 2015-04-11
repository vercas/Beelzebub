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

#include <elf64.h>
#include <jegudiel.h>
#include <info.h>
#include <kernel.h>
#include <lapic.h>
#include <page.h>
#include <screen.h>
#include <stdint.h>
#include <string.h>
#include <idt.h>
#include <gdt.h>

void *kernel_binary = 0;
jg_header_root_t *kernel_header = 0;

void kernel_find(void)
{
    size_t i;

    for (i = 0; i < info_root->module_count; ++i) {
        jg_info_module_t *mod = &info_module[i];
        char *name = &info_strings[mod->name];

        if (strstr(name, KERNEL_NAME) == name) {
            kernel_binary = (void *) mod->address;
            return;
        }
    }

    SCREEN_PANIC("Could not find kernel binary.");
}

void kernel_check(void)
{
    elf64_ehdr_t *ehdr = (elf64_ehdr_t *) kernel_binary;

    if (ehdr->e_ident_mag != ELFMAG ||
        ehdr->e_ident_class != ELFCLASS64 ||
        ehdr->e_type != ELF_ET_EXEC) {
        SCREEN_PANIC("Kernel is not an ELF64 executable.");
    }
}

void kernel_analyze(void)
{
    elf64_sym_t *sym = elf64_sym_find(JG_HEADER_SYMNAME, kernel_binary);

    if (0 == sym) {
        SCREEN_PANIC("The kernel binary does not provide a Jegudiel header.");
    }

    kernel_header = (jg_header_root_t *) sym->st_value;

    if (kernel_header->magic != JG_MAGIC) {
        SCREEN_PANIC("Invalid magic value in kernel header.");
    }
}

void kernel_map_stack(void)
{
    if (0 == kernel_header->stack_vaddr)
        return;

    if (0 != (kernel_header->stack_vaddr & 0xFFF)) {
        SCREEN_PANIC("Virtual stack address in kernel header not page-aligned.");
    }

    uintptr_t stack_begin, stack_address, stack_offset;
    asm volatile ("mov %%rsp, %0" : "=a" (stack_address));
    stack_begin = (stack_address - 1) & ~0xFFF;
    stack_offset = stack_address - stack_begin;

    uintptr_t stack_target = kernel_header->stack_vaddr + 0x1000 * lapic_id();
    page_map(stack_address, stack_target, PAGE_FLAG_WRITABLE | PAGE_FLAG_GLOBAL);

    asm volatile ("mov %0, %%rsp" :: "a" (stack_target + stack_offset));
}

void kernel_map_info(void)
{
    if (0 == kernel_header->info_vaddr)
        return;

    size_t length = info_root->length;
    size_t offset;

    uintptr_t pjgsical = (uintptr_t) info_root;
    uintptr_t virtual = kernel_header->info_vaddr;

    for (offset = 0; offset < length; offset += 0x1000) {
        page_map(pjgsical + offset, virtual + offset, PAGE_FLAG_WRITABLE | PAGE_FLAG_GLOBAL);
    }
}

void kernel_map_idt(void)
{
    if (0 == kernel_header->idt_vaddr)
        return;
        
    uintptr_t pjgs = (uintptr_t) &idt_data;

    page_map(
        pjgs,
        kernel_header->idt_vaddr,
        PAGE_FLAG_WRITABLE | PAGE_FLAG_GLOBAL);
        
    idt_address = kernel_header->idt_vaddr;
}

void kernel_map_gdt(void)
{
    if (0 == kernel_header->gdt_vaddr)
        return;
        
    uintptr_t pjgs = gdt_pointer.address;

    page_map(
        pjgs,
        kernel_header->gdt_vaddr,
        PAGE_FLAG_WRITABLE | PAGE_FLAG_GLOBAL);    
 
    gdt_pointer.address = kernel_header->gdt_vaddr;
}

extern void kernel_enter(uintptr_t address);

void kernel_enter_bsp(void)
{
    kernel_enter(((elf64_ehdr_t *) kernel_binary)->e_entry);
}

void kernel_enter_ap(void)
{
    if (0 == kernel_header->ap_entry) {
        while (1) { asm volatile ("hlt"); }
    } else {
        kernel_enter(kernel_header->ap_entry);
    }
}
