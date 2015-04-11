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

#include <jegudiel.h>
#include <idt.h>
#include <info.h>
#include <kernel.h>
#include <screen.h>
#include <stdint.h>

uintptr_t idt_address = (uintptr_t) &idt_data;

static void idt_fault_pf(void)
{
    asm volatile ("cli");
    SCREEN_PANIC("Page fault.");
}

static void idt_fault_gp(void)
{
    asm volatile ("cli");
    SCREEN_PANIC("General Protection fault.");
}

void idt_load(uintptr_t address, size_t length)
{
    idt_pointer_t pointer = {length - 1, address};
    asm volatile ("lidt (%0)" :: "a" (&pointer));
}

void idt_intgate(idt_entry_t *entry, uintptr_t handler, uint16_t cs, uint8_t dpl)
{
    entry->handlerLow = handler & 0xFFFF;
    entry->handlerMiddle = (handler >> 16) & 0xFFFF;
    entry->handlerHigh = (handler >> 32) & 0xFFFFFFFF;
    
    entry->cs = cs;
    
    entry->flags = 0xE;                 // interrupt gate
    entry->flags |= (dpl & 0b11) << 5;  // dpl
    entry->flags |= (1 << 7);           // present
    
    entry->ist = 0;
    entry->zero = 0;
}

void idt_setup_loader(void)
{
    size_t i;
    for (i = 0; i < IDT_MAX; ++i) {
        idt_intgate(&idt_data[i], (uintptr_t) &idt_null_handler, 0x8, 0x0);
    }
    
    idt_intgate(&idt_data[14], (uintptr_t) &idt_fault_pf, 0x8, 0x0);
    idt_intgate(&idt_data[13], (uintptr_t) &idt_fault_gp, 0x8, 0x0);
}

void idt_setup_kernel(void)
{
    bool present = (0 != kernel_header->isr_entry_table);
    uint64_t *table = (uint64_t *) kernel_header->isr_entry_table;

    size_t i;
    for (i = 0; i < IDT_MAX; ++i) {
        uint64_t handler = (present ? table[i] : 0);
        idt_intgate(&idt_data[i], handler, 0x8, 0x0);
    }
}
