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
#include <apic.h>
#include <jegudiel.h>
#include <stdint.h>

// Byte offsets in IO APIC MMIO section
#define IOAPIC_MMIO_REGSEL      0x0     //< offset of register selector
#define IOAPIC_MMIO_IOWIN       0x10    //< offset of I/O window

// Macros for accessing the MMIO section (p is of type jg_info_ioapic_t *)
#define IOAPIC_MMIO_REGSEL_PTR(p)   (uint32_t *) ((p)->mmio_paddr + IOAPIC_MMIO_REGSEL)
#define IOAPIC_MMIO_IOWIN_PTR(p)    (uint32_t *)((p)->mmio_paddr + IOAPIC_MMIO_IOWIN)

// Bit offsets of components in IO APIC redirection registers
#define IOAPIC_REDIR_VECTOR     0
#define IOAPIC_REDIR_DVL_MODE   8
#define IOAPIC_REDIR_DEST_MODE  11
#define IOAPIC_REDIR_DLV_STATUS 12
#define IOAPIC_REDIR_POL        13
#define IOAPIC_REDIR_RIRR       14
#define IOAPIC_REDIR_TRIGGER    15
#define IOAPIC_REDIR_MASK       16
#define IOAPIC_REDIR_DEST       56

// IO APIC register indices
#define IOAPIC_REG_ID           0x00
#define IOAPIC_REG_VERSION      0x01
#define IOAPIC_REG_ARBITRATION  0x02
#define IOAPIC_REG_REDTBL       0x10

// PCI GSIs
#define IOAPIC_PCI_INTA         17
#define IOAPIC_PCI_INTB         18
#define IOAPIC_PCI_INTC         19
#define IOAPIC_PCI_INTD         20

/**
 * Default redirection entry for loader setup.
 *
 * Fixed delivery to a single CPU. Masked by default. Vector must be OR'ed.
 */
#define IOAPIC_REDIR_LOADER                                ( \
        (APIC_DELIVERY_FIXED    << IOAPIC_REDIR_DVL_MODE)   | \
        (1                      << IOAPIC_REDIR_MASK)       | \
        (APIC_MODE_PHYSICAL     << IOAPIC_REDIR_DEST_MODE)  )

/**
 * Default redirection entry for kernel setup.
 *
 * Delivery to lowest priority CPU. Vector must be OR'ed.
 */
#define IOAPIC_REDIR_KERNEL                                ( \
        (APIC_DELIVERY_LOW_PRIO << IOAPIC_REDIR_DVL_MODE)   | \
        (APIC_MODE_LOGICAL      << IOAPIC_REDIR_DEST_MODE)  | \
        ((uint64_t) 0xFF        << IOAPIC_REDIR_DEST)       )

/**
 * Default redirection entry for kernel setup (with single IOAPIC BSP flag).
 *
 * Delivery to a single CPU. Vector must be OR'ed.
 */
#define IOAPIC_REDIR_KERNEL_BSP                            ( \
        (APIC_DELIVERY_FIXED    << IOAPIC_REDIR_DVL_MODE)   | \
        (APIC_MODE_PHYSICAL     << IOAPIC_REDIR_DEST_MODE)  )

/**
 * Reads an IO APIC register, given its index.
 *
 * @param index The index of the register to read-
 * @param ioapic The IO APIC whose register to read.
 * @return The value of the register.
 */
uint32_t ioapic_register_read(uint16_t index, jg_info_ioapic_t *ioapic);

/**
 * Writes an IO APIC register, given its index and new value.
 *
 * @param index The index of the register to write.
 * @param value The new value for the register.
 * @param ioapic The IO APIC whose register to write.
 * @return The previous value of the register.
 */
uint32_t ioapic_register_write(uint16_t index, uint32_t value, jg_info_ioapic_t *ioapic);

/**
 * Returns an redirection entry, given its index.
 *
 * Behavior is undefined, when the entry does not exist.
 *
 * @param index The index of the redirection entry to return.
 * @param ioapic The IO APIC whose redirection entry to return.
 * @return The redirection entry.
 */
uint64_t ioapic_redir_get(uint16_t index, jg_info_ioapic_t *ioapic);

/**
 * Sets an redirection entry, given its index.
 *
 * @param index The index of the redirection entry to set.
 * @param redir The redirection entry to set.
 * @param ioapic The IO APIC whose redirection entry to set.
 */
void ioapic_redir_set(uint16_t index, uint64_t redir, jg_info_ioapic_t *ioapic);

/**
 * Returns the IO APIC that handles a given <gsi> number.
 *
 * @param gsi the GSI number to return the IO APIC for.
 */
jg_info_ioapic_t *ioapic_get_by_gsi(uint32_t gsi);

/**
 * Analyzes the system's IO APICs and adds the findings to the info tables.
 */
void ioapic_analyze(void);

/**
 * Creates a redirection entry.
 *
 * @param gsi the GSI number of the redirection
 * @param kernel whether to use the entry for the kernel or the loader
 * @return redirection entry
 */
uint64_t ioapic_redir_forge(uint32_t gsi, bool kernel);

/**
 * Sets up the IO APIC and its redirections for the jump into the kernel by
 * transferring them into a well-defined state.
 */
void ioapic_setup_kernel(void);

/**
 * Sets up the IO APIC and its redirections for the initializations that
 * are to be performed by this loader.
 */
void ioapic_setup_loader(void);
