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
#include <info.h>
#include <ioapic.h>
#include <kernel.h>
#include <pit.h>
#include <stdint.h>

static int8_t ioapic_irq_by_gsi(uint32_t gsi)
{
    size_t irq;
    for (irq = 0; irq < 16; ++irq) {
        if (info_root->irq_gsi[irq] == gsi) {
            return irq;
        }
    }

    return -1;
}

static void ioapic_setup(bool kernel)
{
    size_t i, j;
    for (i = 0; i < info_root->ioapic_count; ++i) {
        jg_info_ioapic_t *ioapic = &info_ioapic[i];

        for (j = 0; j < ioapic->gsi_count; ++j) {
            uint32_t gsi = ioapic->gsi_base + j;
            ioapic_redir_set(j, ioapic_redir_forge(gsi, kernel), ioapic);
        }
    }
}

jg_info_ioapic_t *ioapic_get_by_gsi(uint32_t gsi)
{
    size_t i;

    for (i = 0; i < info_root->ioapic_count; ++i) {
        jg_info_ioapic_t *ioapic = &info_ioapic[i];

        uint32_t start = ioapic->gsi_base;
        uint32_t end = start + ioapic->gsi_count;

        if (gsi >= start && gsi < end)
            return ioapic;
    }

    return 0;
}

uint32_t ioapic_register_read(uint16_t index, jg_info_ioapic_t *ioapic)
{
    *IOAPIC_MMIO_REGSEL_PTR(ioapic) = index;
    return *IOAPIC_MMIO_IOWIN_PTR(ioapic);
}

uint32_t ioapic_register_write(uint16_t index, uint32_t value, jg_info_ioapic_t *ioapic)
{
    *IOAPIC_MMIO_REGSEL_PTR(ioapic) = index;
    uint32_t old = *IOAPIC_MMIO_IOWIN_PTR(ioapic);
    *IOAPIC_MMIO_IOWIN_PTR(ioapic) = value;

    return old;
}

uint64_t ioapic_redir_get(uint16_t index, jg_info_ioapic_t *ioapic)
{
    uint16_t register_index = IOAPIC_REG_REDTBL + index * 2;

    uint32_t low = ioapic_register_read(register_index, ioapic);
    uint32_t high = ioapic_register_read(register_index + 1, ioapic);

    return low | ((uint64_t) high << 32);
}

void ioapic_redir_set(uint16_t index, uint64_t redir, jg_info_ioapic_t *ioapic)
{
    uint16_t register_index = IOAPIC_REG_REDTBL + index * 2;

    ioapic_register_write(register_index, redir, ioapic);
    ioapic_register_write(register_index + 1, redir >> 32, ioapic);
}

void ioapic_analyze(void)
{
    size_t i;

    for (i = 0; i < info_root->ioapic_count; ++i) {
        jg_info_ioapic_t *ioapic = &info_ioapic[i];

        uint32_t version_reg = ioapic_register_read(IOAPIC_REG_VERSION, ioapic);

        uint8_t version = version_reg & 0xFF;
        uint8_t max_entry = (version_reg >> 16) & 0xFF;

        ioapic->version = version;
        ioapic->gsi_count = max_entry + 1;
    }
}

uint64_t ioapic_redir_forge(uint32_t gsi, bool kernel)
{
    uint64_t redir = IOAPIC_REDIR_LOADER;

    if (kernel) {
        redir = (0 == (kernel_header->flags & JG_HEADER_FLAG_IOAPIC_BSP))
                ? IOAPIC_REDIR_KERNEL : IOAPIC_REDIR_KERNEL_BSP;
    }

    int8_t irq = ioapic_irq_by_gsi(gsi);

    if (irq >= 0) {
        uint8_t flags = info_root->irq_flags[(size_t) irq];

        if (0 != (JG_INFO_IRQ_FLAG_ACTIVE_LOW & flags)) {
            redir |= (APIC_POLARITY_LOW << IOAPIC_REDIR_POL);
        } else {
            redir |= (APIC_POLARITY_HIGH << IOAPIC_REDIR_POL);
        }

        redir |= (APIC_POLARITY_HIGH << IOAPIC_REDIR_POL);

        if (0 != (JG_INFO_IRQ_FLAG_LEVEL & flags)) {
            redir |= (APIC_TRIGGER_LEVEL << IOAPIC_REDIR_TRIGGER);
        } else {
            redir |= (APIC_TRIGGER_EDGE << IOAPIC_REDIR_TRIGGER);
        }
    } else {
        // Assume its a PCI-like interrupt line then
        redir |= (APIC_TRIGGER_LEVEL << IOAPIC_REDIR_TRIGGER);
        redir |= (APIC_POLARITY_LOW << IOAPIC_REDIR_POL);
    }

    if (kernel) {
        bool mask = 1;
        uint8_t vector = 0;

        if (irq >= 0) {
            jg_header_irq_t *config = &kernel_header->irqs[(size_t) irq];
            mask = (0 != (config->flags & JG_HEADER_IRQ_FLAG_MASK));
            vector = config->vector;
        }

        redir |= ((uint64_t) mask << IOAPIC_REDIR_MASK);
        redir |= vector;

    } else {
        redir |= (1 << IOAPIC_REDIR_MASK);
    }

    return redir;
}

void ioapic_setup_kernel(void)
{
    ioapic_setup(true);
}

void ioapic_setup_loader(void)
{
    ioapic_setup(false);
}
