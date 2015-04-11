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
 * The RSDP is a pointer to the RSDT/XSDT.
 *
 * Can be discovered by searching for its signature, 'RSD PTR' in low memory
 * on BIOS systems, or using the information provided by EFI on EFI enabled
 * systems.
 *
 * Has two versions:
 * 	1.0 Denoted by a value of zero (0) in the revision field; the RSDP is fixed
 * 		to a size of 20 bytes and points to an RSDT.
 * 	2.0 Denoted by a value of one (1) in the revision field; the RSDP has is
 * 		variably-sized (see the length field) and points to an XSDT.
 *
 * If any version later than 2.0 is encounted, it will be treated as of being
 * of version 2.0.
 */
typedef struct acpi_rsdp {
    uint8_t signature[8];
    uint8_t checksum;
    uint8_t oemid[6];
    uint8_t revision;
    uint32_t rsdt_addr;

    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t xchecksum;
    uint8_t reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

/**
 * Each ACPI table (including the RSDT and XSDT) begins with the SDT header.
 */
typedef struct acpi_sdt_header {
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oemid[6];
    uint8_t oem_tbl_id[8];
    uint32_t oem_rev;
    uint32_t creator_id;
    uint32_t creator_rev;
} __attribute__((packed)) acpi_sdt_header_t;

/**
 * Flag in the MADT denoting the presence of a 8259 PIC in the system.
 */
#define ACPI_MADT_PCAT_COMPAT           (1 << 0)

/**
 * Flag in the LAPIC MADT entry indicating that the LAPIC is enabled.
 *
 * Disabled LAPICs should be ignored by the operating system.
 */
#define ACPI_MADT_LAPIC_ENABLED         (1 << 0)

// Polarity flags in ISOs
#define ACPI_MADT_ISO_POLARITY_OFFSET   0
#define ACPI_MADT_ISO_POLARITY_DEFAULT  (0b00)
#define ACPI_MADT_ISO_POLARITY_HIGH     (0b01)
#define ACPI_MADT_ISO_POLARITY_LOW      (0b11)

// Trigger mode flags in ISOs
#define ACPI_MADT_ISO_TRIGGER_OFFSET    2
#define ACPI_MADT_ISO_TRIGGER_DEFAULT   (0b00)
#define ACPI_MADT_ISO_TRIGGER_EDGE      (0b01)
#define ACPI_MADT_ISO_TRIGGER_LEVEL     (0b11)

/**
 * MADT entry type ID for LAPICs, IO APICs and Interrupt Source Overrides.
 */
#define ACPI_MADT_TYPE_LAPIC            0
#define ACPI_MADT_TYPE_IOAPIC           1
#define ACPI_MADT_TYPE_ISO              2
#define ACPI_MADT_TYPE_X2LAPIC          9

/**
 * The Multiple APIC Description Table contains information about the interrupt
 * controllers installed to the system (such as LAPICs and IO APICs).
 *
 * This structure is followed by a variable sized list of APIC devices.
 */
typedef struct acpi_madt {
    acpi_sdt_header_t header;

    uint32_t lapic_paddr;
    uint32_t flags;
} __attribute__((packed)) acpi_madt_t;

/**
 * Header of an entry in the MADT.
 */
typedef struct acpi_madt_entry {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) acpi_madt_entry_t;

/**
 * MADT entry describing a LAPIC and its associated processor.
 */
typedef struct acpi_madt_lapic {
    acpi_madt_entry_t header;

    uint8_t acpi_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) acpi_madt_lapic_t;

/**
 * MADT entry describing an X2APIC LAPIC and its associated processor.
 */
typedef struct apci_madt_x2lapic {
    acpi_madt_entry_t header;

    uint16_t reserved;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__((packed)) acpi_madt_x2lapic_t;

/**
 * MADT entry describing an I/O APIC.
 */
typedef struct acpi_madt_ioapic {
    acpi_madt_entry_t header;

    uint8_t apic_id;
    uint8_t reserved;
    uint32_t mmio_addr;
    uint32_t gsi_base;
} __attribute__((packed)) acpi_madt_ioapic_t;

/**
 * MADT entry for interrupt source overrides.
 *
 * Denotes non-standard mappings of ISA IRQs to Global System Interrupts.
 */
typedef struct acpi_madt_iso {
    acpi_madt_entry_t header;

    uint8_t bus;
    uint8_t irq;
    uint32_t gsi;
    uint16_t flags;
} __attribute__((packed)) acpi_madt_iso_t;

/**
 * Flag in the LAPIC and x2LAPIC SRAT entries that indicates that the entry is
 * enabled and should be parsed. If this flag is clear, the entry must be ignored.
 */
#define ACPI_SRAT_LAPIC_ENABLED     (1 << 0)

// SRAT entry types.
#define ACPI_SRAT_TYPE_LAPIC        0
#define ACPI_SRAT_TYPE_MEMORY       1
#define ACPI_SRAT_TYPE_X2LAPIC      2

/**
 * The System Resource Affinity Table maps the system's CPUs to the NUMA domains
 * and the domains to the memory regions that are associated with them.
 *
 * This structure is followed by a variable sized list of entries.
 */
typedef struct acpi_srat {
    acpi_sdt_header_t header;

    uint32_t reserved0;
    uint64_t reserved1;
} __attribute__((packed)) acpi_srat_t;

/**
 * Header of an entry in the SRAT.
 */
typedef struct acpi_srat_entry {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) acpi_srat_entry_t;

/**
 * SRAT table entry that associates a CPU's LAPIC to a NUMA domain.
 */
typedef struct acpi_srat_lapic {
    acpi_srat_entry_t header;

    uint8_t domain_low;
    uint8_t apic_id;
    uint32_t flags;
    uint8_t sapic_eid;
    uint8_t domain_high[3];
    uint32_t clock_domain;
} __attribute__((packed)) acpi_srat_lapic_t;

/**
 * SRAT table entry that associates a CPU's x2LAPIC to a NUMA domain.
 */
typedef struct acpi_srat_x2lapic {
    acpi_srat_entry_t header;

    uint16_t reserved0;
    uint32_t domain;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t clock_domain;
    uint32_t reserved1;
} __attribute__((packed)) acpi_srat_x2lapic_t;

/**
 * SRAT table entry that associates a memory range to a NUMA domain.
 */
typedef struct acpi_srat_memory {
    acpi_srat_entry_t header;

    uint32_t domain;
    uint16_t reserved0;
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t reserved1;
    uint32_t flags;
    uint64_t reserved2;
} __attribute__((packed)) acpi_srat_memory_t;

/**
 * Searches for the RSDP on a 16 byte boundary, given a memory region to
 * search in.
 *
 * @param begin The address on which the region to search begins.
 * @param length The length of the region.
 * @return Pointer to the RSDP or null-pointer if it could not be found.
 */
acpi_rsdp_t *acpi_find_rsdp(uintptr_t begin, size_t length);

/**
 * Validates the checksum of a table, given its length.
 *
 * Adds up all bytes of the table and checks whether the sum modulo 256
 * equals zero (0).
 *
 * @param table The address of the table whose checksum to validate.
 * @param length The length of the table.
 * @return <tt>true</tt> when the table is valid, <tt>false</tt> otherwise.
 */
bool acpi_check(void *table, size_t length);

/**
 * Discovers the system's ACPI info tables and extracts all vital information
 * that is required to fill the Jegudiel info tables.
 */
void acpi_parse(void);
