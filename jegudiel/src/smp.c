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
#include <jegudiel.h>
#include <info.h>
#include <lapic.h>
#include <smp.h>
#include <stdint.h>
#include <string.h>

volatile uint64_t smp_ready_count;

static void smp_boot(jg_info_cpu_t *cpu)
{
    // Send INIT IPI
    lapic_ipi(LAPIC_IPI_INIT, cpu->apic_id);

    // Wait a moment (10ms)
    lapic_timer_wait(10 * 1000);

    // Backup ready count
    uint64_t ready_new = smp_ready_count + 1;

    // Send STARTUP IPI
    lapic_ipi(LAPIC_IPI_STARTUP(0x1000), cpu->apic_id);

    // One second timeout
    for (int i = 0; i < 10; ++i)
    {
        // wait 100 ms
        lapic_timer_wait(100 * 1000);
        if (ready_new == smp_ready_count)
            return;
    }

    //while (ready_new != smp_ready_count);
}

static void smp_prepare_boot16(void)
{
    extern uint8_t boot16_begin;
    extern uint8_t boot16_end;

    size_t length = ((uintptr_t) &boot16_end - (uintptr_t) &boot16_begin);
    memcpy((void *) SMP_BOOT16_TARGET, &boot16_begin, length);
}

void smp_setup(void)
{
    smp_prepare_boot16();

    size_t i;
    for (i = 0; i < info_root->cpu_count; ++i) {
        jg_info_cpu_t *cpu = &info_cpu[i];

        if (0 == (cpu->flags & JG_INFO_CPU_FLAG_PRESENT))
            continue;

        if (0 != (cpu->flags & JG_INFO_CPU_FLAG_BSP))
            continue;

        smp_boot(cpu);
    }
}
