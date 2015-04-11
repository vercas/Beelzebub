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

#include <buffer.h>
#include <jegudiel.h>
#include <isr.h>
#include <keyboard.h>
#include <screen.h>
#include <stdint.h>
#include <ui.h>

#include <lapic.h>
#include <screen.h>

uint64_t pit_ticks = 0;

static char *build_overview(char *buffer)
{
    ui_pages[0].title = "Overview";
    ui_pages[0].body = buffer;

    jg_info_root_t *root = JG_INFO_ROOT;

    BSTR("Welcome to the H2 Test Utility.\n");
    BSTR("Use LEFT and RIGHT to switch between pages, and UP and DOWN to scroll.\n\n");

    bool pic = (0 != (root->flags & JG_INFO_FLAG_PCAT_COMPAT));
    bool x2apic = (0 != (root->flags & JG_INFO_FLAG_X2APIC));

    BSTR("8259 PIC present:   ");
    BSTR(pic ? "Yes" : "No");
    BSTR("\nx2APIC present:     ");
    BSTR(x2apic ? "Yes" : "No");
    BSTR("\n\n");

    BSTR("LAPIC MMIO:         ");
    BNUM(root->lapic_paddr);
    BSTR("\nRSDP Address:       ");
    BNUM(root->rsdp_paddr);
    BSTR("\nFirst Free Address: ");
    BNUM(root->free_paddr);
    BSTR("\n\n");

    return buffer;
}

static char *build_cpu(char *buffer)
{
    ui_pages[1].title = "CPUs";
    ui_pages[1].body = buffer;

    size_t i;
    for (i = 0; i < JG_INFO_ROOT->cpu_count; ++i) {
        jg_info_cpu_t *cpu = &(JG_INFO_CPU[i]);

        if (0 == (cpu->flags & JG_INFO_CPU_FLAG_PRESENT))
            continue;

        BSTR("APIC ID:           ");
        BNUM(cpu->apic_id);
        BSTR("\nACPI ID:           ");
        BNUM(cpu->acpi_id);
        BSTR("\nLAPIC Timer Freq.: ");
        BNUM(cpu->lapic_timer_freq);
        BSTR(" Hz");
        BSTR("\nNUMA domain:       ");
        BNUM(cpu->domain);
        BSTR("\n\n");
    }

    return buffer;
}

static char *build_ioapic(char *buffer)
{
    ui_pages[2].title = "IO APICs";
    ui_pages[2].body = buffer;

    size_t i;
    for (i = 0; i < JG_INFO_ROOT->ioapic_count; ++i) {
        jg_info_ioapic_t *ioapic = &(JG_INFO_IOAPIC[i]);

        BSTR("APIC ID:      ");
        BNUM(ioapic->apic_id);
        BSTR("\nVersion:      ");
        BNUM(ioapic->version);
        BSTR("\nGSI Base:     ");
        BNUM(ioapic->gsi_base);
        BSTR("\nGSI Count:    ");
        BNUM(ioapic->gsi_count);
        BSTR("\nMMIO Address: ");
        BNUM(ioapic->mmio_paddr);
        BSTR("\n\n");
    }

    return buffer;
}

static char *build_memory(char *buffer)
{
    ui_pages[3].title = "Memory Map";
    ui_pages[3].body = buffer;

    size_t i;
    for (i = 0; i < JG_INFO_ROOT->mmap_count; ++i) {
        jg_info_mmap_t *mmap = &JG_INFO_MMAP[i];

        BSTR("Address:   ");
        BNUM(mmap->address);
        BSTR("\nLength:    ");
        BNUM(mmap->length);
        BSTR("\nAvailable: ");
        BSTR((1 == mmap->available) ? "Yes" : "No");
        BSTR("\n\n");
    }

    return buffer;
}

static char *build_modules(char *buffer)
{
    ui_pages[4].title = "Modules";
    ui_pages[4].body = buffer;

    size_t i;
    for (i = 0; i < JG_INFO_ROOT->module_count; ++i) {
        jg_info_module_t *mod = &JG_INFO_MODULE[i];

        BSTR("Name:    ");
        BSTR(&JG_INFO_STRING[mod->name]);
        BSTR("\nAddress: ");
        BNUM(mod->address);
        BSTR("\nLength:  ");
        BNUM(mod->length);
        BSTR("\n\n");
    }

    return buffer;
}

static void fault_gp(isr_state_t *state)
{
    char buffer_data[50];
    char *buffer = (char *) &buffer_data;
    buffer = buffer_write("INT#", buffer);
    buffer = buffer_write_hex(state->vector, buffer);
    screen_write(buffer, 10, 20);
}

void kmain_bsp(void);
void kmain_bsp(void)
{
    size_t i = 0;
    for (i = 0; i < 256; ++i) {
        isr_handlers[i] = (uintptr_t) &fault_gp;
    }
    isr_handlers[KEYBOARD_IRQ_VECTOR] = (uintptr_t) &keyboard_handler;

    char *buffer = (char *) JG_INFO_ROOT->free_paddr;
    buffer = build_overview(buffer);
    buffer = build_cpu(&buffer[1]);
    buffer = build_ioapic(&buffer[1]);
    buffer = build_memory(&buffer[1]);
    buffer = build_modules(&buffer[1]);

    ui_display(0, 0);
    asm volatile ("sti");
    keyboard_init();
    while (1);
}
