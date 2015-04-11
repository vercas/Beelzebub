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

// Value for elf64_ehdr.e_ident_mag
#define ELFMAG                  0x464C457F  //< 0x7F + 'ELF'

// Values for elf64_ehdr.e_indent_class
#define ELFCLASSNONE            0           //< unknown
#define ELFCLASS32              1           //< 32 bit arch
#define ELFCLASS64              2           //< 64 bit arch

// Values for elf64_ehdr.e_ident_data
#define ELFDATANONE             0           //< unknown
#define ELFDATA2LSB             1           //< little endian
#define ELFDATA2MSB             2           //< big endian

// Values for elf64_ehdr.e_type
#define ELF_ET_NONE             0           //< unknown
#define ELF_ET_REL              1           //< relocatable
#define ELF_ET_EXEC             2           //< executable
#define ELF_ET_DYN              3           //< shared object
#define ELF_ET_CORE             4           //< core file

// Values for elf64_phdr.p_type
#define ELF_PT_NULL             0           //< unused entry
#define ELF_PT_LOAD             1           //< loadable segment
#define ELF_PT_DYNAMIC          2           //< dynamic linking information segment
#define ELF_PT_INTERP           3           //< pathname of interpreter
#define ELF_PT_NOTE             4           //< auxiliary information
#define ELF_PT_SHLIB            5           //< reserved (not used)
#define ELF_PT_PHDR             6           //< location of program header itself

// Values for elf64_phdr.p_flags
#define ELF_PF_X                (1 << 0)    //< executable
#define ELF_PF_W                (1 << 1)    //< writable
#define ELF_PF_R                (1 << 2)    //< readable

// Values for elf64_shdr.sh_type
#define ELF_SHT_NULL            0           //< inactive
#define ELF_SHT_PROGBITS        1           //< program defined information
#define ELF_SHT_SYMTAB          2           //< symbol table section
#define ELF_SHT_STRTAB          3           //< string table section
#define ELF_SHT_RELA            4           //< relocation section with addends
#define ELF_SHT_HASH            5           //< symbol hash table section
#define ELF_SHT_DYNAMIC         6           //< dynamic section
#define ELF_SHT_NOTE            7           //< note section
#define ELF_SHT_NOBITS          8           //< no space section
#define ELF_SHT_REL             9           //< relation section without addends
#define ELF_SHT_SHLIB           10          //< reserved - purpose unknown
#define ELF_SHT_DYNSYM          11          //< dynamic symbol table section
#define ELF_SHT_LOPROC          0x70000000  //< reserved range for processor
#define ELF_SHT_HIPROC          0x7FFFFFFF  //< specific section header types
#define ELF_SHT_LOUSER          0x80000000  //< reserved range for application
#define ELF_SHT_HIUSER          0xFFFFFFFF  //< specific indexes

// Values for elf64_dyn.d_type
#define ELF_DT_NULL             0
#define ELF_DT_HASH             4           //< address of the symbol hash table

/**
 * ELF64 file header.
 */
typedef struct elf64_ehdr {
    uint32_t e_ident_mag;
    uint8_t e_ident_class;
    uint8_t e_ident_data;
    uint8_t e_ident_version;
    uint8_t e_ident_osabi;
    uint8_t e_ident_abiversion;
    uint8_t e_ident_pad[7];

    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;

    uint16_t e_ehsize;
    uint16_t e_phsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf64_ehdr_t;

/**
 * ELF64 program header.
 *
 * Contains information on where to load the binary's data and code from and where
 * to map it to.
 */
typedef struct elf64_phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) elf64_phdr_t;

/**
 * ELF64 section header.
 */
typedef struct elf64_shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} __attribute__((packed)) elf64_shdr_t;

/**
 * ELF64 symbol table entry.
 */
typedef struct elf64_sym {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} __attribute__((packed)) elf64_sym_t;

/**
 * ELF64 dynamic table entry.
 */
typedef struct elf64_dyn {
    uint64_t d_type;

    union {
        uint64_t d_val;
        uint64_t d_ptr;
    } d_un;
} __attribute__((packed)) elf64_dyn_t;

/**
 * Calculates a hash value for a symbol <name>.
 *
 * @param name the name to generate a hash for.
 * @return the hash of the name
 */
uint64_t elf64_hash(const char *name);

/**
 * Returns the header of the first section of the given <type> in an ELF64 <binary>.
 *
 * @param type the type of the section
 * @param binary the ELF64 binary
 * @return header of the first matching section
 */
elf64_shdr_t *elf64_shdr_find(uint32_t type, void *binary);

/**
 * Tries to find a symbol in an ELF64 <binary>, given its <name>.
 *
 * @param name the name of the symbol to find
 * @param binary the ELF64 binary
 * @return pointer to the symbol entry or null pointer, if there is no such symbol
 */
elf64_sym_t *elf64_sym_find(const char *name, void *binary);

/**
 * Loads an ELF64 <binary> into virtual memory.
 *
 * @param binary the binary to load
 */
void elf64_load(void *binary);
