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
#include <arc/lapic.h>
#include <stdint.h>

#include <arc/screen.h>

#define LAPIC_X2APIC_MODE (0 != (JG_INFO_ROOT->flags & JG_INFO_FLAG_X2APIC))

static uint64_t __msr_read(uint32_t msr)
{
    uint32_t a, d;
    asm volatile ("rdmsr" : "=a" (a), "=d" (d) : "c" (msr));

    return (((uint64_t) d) << 32) | a;
}

static void __msr_write(uint32_t msr, uint64_t value)
{
    uint32_t a = value;
    uint32_t d = (value >> 32);

    asm volatile ("wrmsr" :: "c" (msr), "a" (a), "d" (d));
}

uint32_t lapic_register_read(uint16_t index)
{
    if (!LAPIC_X2APIC_MODE) {
        return *((uint32_t *) (index * 0x10 + JG_INFO_ROOT->lapic_paddr));
    } else {
        return __msr_read(LAPIC_MSR_REGS + index);
    }
}

void lapic_register_write(uint16_t index, uint32_t value)
{
    if (!LAPIC_X2APIC_MODE) {
        *((uint32_t *) (index * 0x10 + JG_INFO_ROOT->lapic_paddr)) = value;
    } else {
        __msr_write(LAPIC_MSR_REGS + index, value);
    }
}
