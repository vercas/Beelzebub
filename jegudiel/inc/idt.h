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

/**
 * Length of the IDT in bytes
 */
#define IDT_LENGTH 0x1000

/**
 * Number of entries in the IDT.
 */
#define IDT_MAX 256

/**
 * Pointer to the Interrupt Descriptor Table that can be loaded using
 * the LIDT instruction.
 */
typedef struct idt_pointer {

    uint16_t length;        //< length of the IDT - 1
    uint64_t address;       //< virtual address of the IDT

} __attribute__((packed)) idt_pointer_t;

/**
 * Entry in the Interrupt Descriptor Table.
 *
 * The handler is the virtual address of the ISR to invoke when the
 * interrupt associated with this entry is raised.
 */
typedef struct idt_entry {

    uint16_t handlerLow;        //< lowest 2 bytes of the handler
    uint16_t cs;                //< code segment selector
    uint8_t ist;                //< offset into the IST
    uint8_t flags;              //< flags
    uint16_t handlerMiddle;     //< middle 2 bytes of the handler
    uint32_t handlerHigh;       //< highest 4 bytes of the handler
    uint32_t zero;              //< cleared

} __attribute__((packed)) idt_entry_t;

/**
 * Data of the Interrupt Descriptor Table.
 *
 * Should be aligned on a page-boundary.
 */
extern idt_entry_t idt_data[IDT_MAX];

/**
 * The address of the IDT to use.
 */
extern uintptr_t idt_address;

/**
 * An ISR that just IRETQs.
 */
void idt_null_handler(void);

/**
 * Loads an interrupt descriptor table, given its <address> and <length>.
 *
 * @param address virtual address of the IDT
 * @param length length of the IDT in bytes
 */
void idt_load(uintptr_t address, size_t length);

/**
 * Prepares an interrupt gate, given the IDT entry to modify.
 * 
 * @param entry IDT entry to modify
 * @param handler virtual address of the ISR
 * @param cs code segment the ISR should run in
 * @param dpl least privileged DPL at which the interrupt can be software triggered
 */
void idt_intgate(idt_entry_t *entry, uintptr_t handler, uint16_t cs, uint8_t dpl);

/**
 * Sets up the IDT for the loader.
 *
 * Sets all entries to interrupt gates with a minimum DPL of 0x0, a code segment
 * of 0x08 and the idt_null_handler as ISR, except for some fault handlers.
 *
 * Can be called multiply times to reset the IDT.
 */
void idt_setup_loader(void);

/**
 * Sets up the IDT for the kernel.
 *
 * Sets all entries in the IDT to be interrupt gates with a minimum DPL of 0x0, a
 * code segment of 0x08 and ISRs as specified by the ISR entry table passed with
 * the kernel header. If the pointer to the ISR entry table is a null-pointer, all
 * ISR addresses will default to zero (0x0).
 */
void idt_setup_kernel(void);
