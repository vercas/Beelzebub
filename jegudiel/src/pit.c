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
#include <ioapic.h>
#include <lapic.h>
#include <pit.h>
#include <ports.h>
#include <stdint.h>

void pit_freq_set(uint16_t freq)
{
    uint16_t divisor = PIT_FREQ_BASE / freq;
    outb(PIT_IO_COUNTER, divisor);
    outb(PIT_IO_COUNTER, divisor >> 8);
}

static void pit_set_redir(uint64_t redir)
{
    uint8_t irq = (0 == (info_root->flags & JG_INFO_FLAG_PCAT_COMPAT)) ? PIT_IRQ : PIT_IRQ_PCAT_COMPAT;
    uint32_t gsi = info_root->irq_gsi[irq];
    jg_info_ioapic_t *ioapic = ioapic_get_by_gsi(gsi);
    uint32_t index = gsi - ioapic->gsi_base;
    ioapic_redir_set(index, redir, ioapic);
}

void pit_route(void)
{
    uint64_t redir = PIT_VECTOR | ((uint64_t) lapic_id() << IOAPIC_REDIR_DEST);
    pit_set_redir(redir);
}

void pit_mask(void)
{
    uint64_t redir = (1 << IOAPIC_REDIR_MASK);
    pit_set_redir(redir);
}
