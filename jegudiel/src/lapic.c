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

#include <apic.h>
#include <cpu.h>
#include <idt.h>
#include <info.h>
#include <ioapic.h>
#include <kernel.h>
#include <lapic.h>
#include <pit.h>
#include <screen.h>
#include <stdint.h>

#define LAPIC_X2APIC_MODE (0 != (JG_INFO_FLAG_X2APIC & info_root->flags))

void lapic_detect(void)
{
    cpu_cpuid_result_t cpuid;
    cpu_cpuid(0x1, &cpuid);
    bool x2apic_supported = (0 != ((1 << 21) & cpuid.c));

    if (0 != (kernel_header->flags & JG_HEADER_FLAG_X2APIC_ALLOW))
    {
        if (x2apic_supported)
        {
            info_root->flags |= JG_INFO_FLAG_X2APIC;

            puts(" x2APIC!");
        }
        else
        {
            puts(" No x2APIC...");

            if (0 != (kernel_header->flags & JG_HEADER_FLAG_X2APIC_REQUIRE))
            {
                SCREEN_PANIC("x2APIC required but not supported.");
            }
        }
    }
}

uint32_t lapic_register_read(uint16_t index)
{
    if (!LAPIC_X2APIC_MODE) {
        return *((uint32_t *) (info_root->lapic_paddr + index * 0x10));
    } else {
        return cpu_msr_read(LAPIC_MSR_REGS + index);
    }
}

void lapic_register_write(uint16_t index, uint32_t value)
{
    if (!LAPIC_X2APIC_MODE) {
        *((uint32_t *) (info_root->lapic_paddr + index * 0x10)) = value;
    } else {
        cpu_msr_write(LAPIC_MSR_REGS + index, value);
    }
}

void lapic_setup(void)
{
    uint64_t base_msr = cpu_msr_read(LAPIC_MSR_BASE);

    bool en = (0 != (base_msr & LAPIC_MSR_BASE_EN));
    bool extd = (0 != (base_msr & LAPIC_MSR_BASE_EXTD));

    if (!en) {
        base_msr |= LAPIC_MSR_BASE_EN;
        cpu_msr_write(LAPIC_MSR_BASE, base_msr);
    }

    if (LAPIC_X2APIC_MODE && !extd) {
        base_msr |= LAPIC_MSR_BASE_EXTD;
        cpu_msr_write(LAPIC_MSR_BASE, base_msr);
    }

    lapic_timer_update(0xFFFFFFFF, 0, 1, 0);
    lapic_register_write(LAPIC_REG_TPR, LAPIC_TPR);
    lapic_register_write(LAPIC_REG_PCINT, LAPIC_PCINT);
    lapic_register_write(LAPIC_REG_LINT0, LAPIC_LINT0);
    lapic_register_write(LAPIC_REG_LINT1, LAPIC_LINT1);
    lapic_register_write(LAPIC_REG_ERRINT, LAPIC_ERRINT);

    if (!LAPIC_X2APIC_MODE) {
        lapic_register_write(LAPIC_REG_LDR, LAPIC_LDR);
    }

    lapic_register_write(LAPIC_REG_SVR, LAPIC_SVR);
}

uint32_t lapic_id(void)
{
    uint32_t id = lapic_register_read(LAPIC_REG_ID);
    return (LAPIC_X2APIC_MODE) ? id : (id >> 24);
}

void lapic_eoi(void)
{
    lapic_register_write(LAPIC_REG_EOI, 0);
}

void lapic_ipi(uint32_t icr_low, uint32_t destination)
{
    if (!LAPIC_X2APIC_MODE) {
        lapic_register_write(LAPIC_REG_ICR_HIGH, (destination & 0xFF) << 24);
        lapic_register_write(LAPIC_REG_ICR_LOW, icr_low);
    } else {
        uint64_t icr = icr_low | ((uint64_t) destination << 32);
        cpu_msr_write(LAPIC_MSR_REGS + LAPIC_REG_ICR_X2APIC, icr);
    }
}

void lapic_timer_update(uint32_t init_count, uint8_t vector, bool mask, bool periodic)
{
    uint32_t lvt = vector | (mask << LAPIC_TIMER_MASK) | (periodic << LAPIC_TIMER_TRIGGER);

    lapic_register_write(LAPIC_REG_TIMER_DIV, 0xB);
    lapic_register_write(LAPIC_REG_TIMER, lvt);
    lapic_register_write(LAPIC_REG_TIMER_INIT, init_count);
}

void lapic_timer_calibrate(void)
{
    extern uint32_t lapic_timer_calibrate_worker(void);
    extern void lapic_timer_calibrate_handler(void);

    uint8_t vector = PIT_VECTOR;

    idt_intgate(&idt_data[vector], (uintptr_t) &lapic_timer_calibrate_handler, 0x08, 0x0);
    pit_freq_set(100);
    pit_route();

    uint32_t ticks_per_second = lapic_timer_calibrate_worker();

    info_cpu[lapic_id()].lapic_timer_freq = ticks_per_second;

    pit_mask();
    idt_setup_loader();
}

void lapic_timer_wait(uint64_t time)
{
    extern void lapic_timer_wait_handler(void);
    extern void lapic_timer_wait_worker(void);

    uint8_t vector = 0x40;
    idt_intgate(&idt_data[vector], (uintptr_t) &lapic_timer_wait_handler, 0x8, 0x0);

    uint64_t ticks = ((uint64_t) info_cpu[lapic_id()].lapic_timer_freq * time) / 1000000;
    lapic_timer_update(ticks, vector, 0, 0);

    lapic_timer_wait_worker();

    idt_setup_loader();
}
