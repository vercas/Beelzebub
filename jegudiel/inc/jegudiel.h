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
 * Jegudiel ABI (Version 1)
 *
 * The ABI for both, the info tables that Jegudiel passes to the kernel, as
 * well as the Jegudiel header in the loaded kernel that configures how the
 * kernel should be loaded. See SPECIFICATION.mdown for a more detailed
 * description.
 * 
 * This file can be included in the kernel in order to access the generated
 * info tables easily. In order to prevent potential name clashes, the name of
 * each structure and macro in this file begins with the prefix "jg_".
 *
 * The values and definitions in this file are subject to change in further
 * versions of Jegudiel, potentially breaking backward compatibility.
 */

//-----------------------------------------------------------------------------
// Common
//-----------------------------------------------------------------------------

/**
 * The magic number for both the Jegudiel info table and the Jegudiel header ("JGDR").
 */
#define JG_MAGIC                0x52445948

//-----------------------------------------------------------------------------
// Info Table - Memory Structure
//-----------------------------------------------------------------------------

#define JG_INFO_OFFSET(name)    ((uintptr_t) (0x14C000 + JG_INFO_ROOT-> name ## _offset))

#define JG_INFO_ROOT            ((jg_info_root_t *) 0x14C000)
#define JG_INFO_CPU             ((jg_info_cpu_t *) JG_INFO_OFFSET(cpu))
#define JG_INFO_IOAPIC          ((jg_info_ioapic_t *) JG_INFO_OFFSET(ioapic))
#define JG_INFO_MMAP            ((jg_info_mmap_t *) JG_INFO_OFFSET(mmap))
#define JG_INFO_MODULE          ((jg_info_module_t *) JG_INFO_OFFSET(module))
#define JG_INFO_STRING          ((char *) JG_INFO_OFFSET(string))

//-----------------------------------------------------------------------------
// Info Table - Flags
//-----------------------------------------------------------------------------

/** CPU Flag: Set when the CPU entry represents an enabled and present CPU. */
#define JG_INFO_CPU_FLAG_PRESENT        (1 << 0)

/** CPU Flag: Set when the CPU entry represents the bootstrap processor. */
#define JG_INFO_CPU_FLAG_BSP            (1 << 1)

/** Root Flag: The system has a 8259 PIC. */
#define JG_INFO_FLAG_PCAT_COMPAT        (1 << 0)

/** Root Flag: The LAPICs are in x2APIC mode. */
#define JG_INFO_FLAG_X2APIC             (1 << 1)

/** IRQ Flag: The IRQ's interrupt line is active low (default: active high). */
#define JG_INFO_IRQ_FLAG_ACTIVE_LOW     (1 << 0)

/** IRQG Flag:The IRQ's interrupt line is level triggered (default: edge). */
#define JG_INFO_IRQ_FLAG_LEVEL          (1 << 1)

//-----------------------------------------------------------------------------
// Info Table - Structures
//-----------------------------------------------------------------------------

/**
 * Jegudiel root info table that contains general information about the system.
 */
typedef struct jg_info_root {
    
    uint32_t magic;             //< a magic number (JG_MAGIC)
    uint32_t flags;             //< flags
    uint16_t length;            //< length of the info tables

    uint64_t lapic_paddr;       //< pjgsical address of the LAPIC MMIO window
    uint64_t rsdp_paddr;        //< pjgsical address of the RSDP (ACPI)
    
    uint64_t idt_paddr;         //< pjgsical address of the IDT
    uint64_t gdt_paddr;         //< pjgsical address of the GDT
    uint64_t tss_paddr;         //< pjgsical address of the TSS entries
    
    uint64_t free_paddr;        //< pjgsical address of the first free to use byte
    
    uint32_t irq_gsi[16];       //< map of ISR IRQ numbers to GSI numbers
    uint8_t irq_flags[16];      //< flags regarding the IRQs
    
    uint16_t cpu_offset;        //< offset of the CPU table
    uint16_t ioapic_offset;     //< offset of the IO APIC table
    uint16_t mmap_offset;       //< offset of the MMAP table
    uint16_t module_offset;     //< offset of the module table
    uint16_t string_offset;     //< offset of the string table

    uint16_t cpu_count_active;  //< number of active CPUs in the system
    uint16_t cpu_count;         //< number of entries in the CPU table
    uint16_t ioapic_count;      //< number of IO APICs
    uint16_t mmap_count;        //< number of entries in the memory map
    uint16_t module_count;      //< number of modules
    
} __attribute__((packed)) jg_info_root_t;

/**
 * An entry in the CPU info table which represents a single CPU in the system.
 * 
 * Without the JG_INFO_CPU_PRESENT flag being set, the CPU entry can be ignored.
 * 
 * Length: 17 bytes.
 */
typedef struct jg_info_cpu {
    uint32_t apic_id;           //< apic id of the CPU's LAPIC
    uint32_t acpi_id;           //< acpi id of the CPU
    uint16_t flags;             //< CPU flags
    uint32_t lapic_timer_freq;  //< lapic timer ticks per second
    uint32_t domain;            //< which NUMA domain the CPU belongs to
} __attribute__((packed)) jg_info_cpu_t;

/**
 * An entry in the IO APIC info table which represents a single IO APIC that
 * is installed into the system and that covers a given interval of GSIs.
 * 
 * Length: 16 bytes.
 */
typedef struct jg_info_ioapic {
    uint8_t apic_id;            //< apic id of the IO APIC
    uint8_t version;			//< version of the IO APIC
    uint32_t gsi_base;          //< lowest GSI covered by this IO APIC
    uint16_t gsi_count;         //< number of GSIs covered by this IO APIC
    uint64_t mmio_paddr;        //< pjgsical address of IO APIC's MMIO window
} __attribute__((packed)) jg_info_ioapic_t;

/**
 * An entry in the memory map, indicating whether a region is free to use as
 * normal memory or is allocated by another device.
 * 
 * Length: 32 bytes.
 */
typedef struct jg_info_mmap {
    uint64_t address;           //< pjgsical address the region begins on
    uint64_t length;            //< length of the region in bytes
    uint64_t available;         //< one if available, zero otherwise
    uint64_t padding;
} __attribute__((packed)) jg_info_mmap_t;

/**
 * An entry in the module list which represents a module loaded into memory.
 * 
 * Length: 16 bytes.
 */
typedef struct jg_info_module {
    uint16_t name;              //< offset of the name in the string table
    uint64_t address;           //< pjgsical address of the module
    uint32_t length;            //< length of the module in bytes
    uint16_t padding;
} __attribute__((packed)) jg_info_module_t;

//-----------------------------------------------------------------------------
// Kernel Header - Symbol Names
//-----------------------------------------------------------------------------

/** The name of the symbol that points to the kernel header. */
#define JG_HEADER_SYMNAME   "jegudiel_header"

//-----------------------------------------------------------------------------
// Kernel Header - Flags
//-----------------------------------------------------------------------------

/** Root Flag: Instead of lowest priority delivery, route all GSIs to the BSP. */
#define JG_HEADER_FLAG_IOAPIC_BSP       (1 << 0)

/** Root Flag: Switch to X2APIC mode, if available. */
#define JG_HEADER_FLAG_X2APIC_ALLOW     (1 << 1)

/** Root Flag: Require X2APIC support, fail otherwise. X2APIC_ALLOW must be set. */
#define JG_HEADER_FLAG_X2APIC_REQUIRE   (1 << 2)

/** IRQ Flag: The IRQ should be masked when the kernel is entered. */
#define JG_HEADER_IRQ_FLAG_MASK         (1 << 0)

//-----------------------------------------------------------------------------
// Kernel Header - Structures
//-----------------------------------------------------------------------------

/**
 * An entry in the IRQ array of the root header structure.
 *
 * Enables the kernel to configure masks and vectors for each IRQ individually.
 */
typedef struct jg_header_irq {
    uint8_t flags;              //< IRQ flags
    uint8_t vector;             //< IRQ vector
} __attribute__((packed)) jg_header_irq_t;

/**
 * The root structure of the kernel header.
 *
 * Enables the kernel to configure various memory mappings and entry points and
 * contains the IRQ array.
 */
typedef struct jg_header_root {
    uint32_t magic;             //< magic value
    uint32_t flags;             //< flags

    uint64_t stack_vaddr;       //< virtual address for the stack (or null)
    uint64_t info_vaddr;        //< virtual address for the info tables (or null)
    uint64_t idt_vaddr;         //< virtual address for the IDT (or null)
    uint64_t gdt_vaddr;         //< virtual address for the GDT (or null)

    uint64_t ap_entry;          //< entry point for APs (or null)
    uint64_t syscall_entry;     //< entry point for syscalls (or null)
    uint64_t isr_entry_table;   //< ISR entry table pointer (or null)

    jg_header_irq_t irqs[16];   //< IRQ configuration
} __attribute__((packed)) jg_header_root_t;
