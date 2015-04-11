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

#include <elf64.h>
#include <heap.h>
#include <page.h>
#include <stdint.h>
#include <string.h>

#include <screen.h>

uint64_t elf64_hash(const char *name)
{
    uint64_t h = 0, g;

    while (*name) {
        h = (h << 4) + *name++;
        g = h & 0xf0000000;

        if (g != 0)
            h ^= g >> 24;

        h &= 0x0fffffff;
    }

    return h;
}

elf64_shdr_t *elf64_shdr_find(uint32_t type, void *binary)
{
    elf64_ehdr_t *ehdr = (elf64_ehdr_t *) binary;
    elf64_shdr_t *shdr_list = (elf64_shdr_t *) ((uintptr_t) binary + ehdr->e_shoff);

    size_t i;
    for (i = 0; i < ehdr->e_shnum; ++i) {
        elf64_shdr_t *shdr = &shdr_list[i];

        if (type == shdr->sh_type) {
            return shdr;
        }
    }

    return 0;
}

elf64_sym_t *elf64_sym_find(const char *name, void *binary)
{
    elf64_ehdr_t *ehdr = (elf64_ehdr_t *) binary;
    elf64_shdr_t *symtab_hdr = elf64_shdr_find(ELF_SHT_SYMTAB, binary);

    if (0 == symtab_hdr) {
        return 0;
    }

    elf64_shdr_t *strtab_hdr = (elf64_shdr_t *) (
            ehdr->e_shoff +
            symtab_hdr->sh_link * ehdr->e_shentsize +
            (uintptr_t) binary);

    char *strtab = (char *) (strtab_hdr->sh_offset + (uintptr_t) binary);
    elf64_sym_t *symtab = (elf64_sym_t *) (symtab_hdr->sh_offset + (uintptr_t) binary);

    size_t i;
    size_t symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;
    for (i = 0; i < symbol_count; ++i) {
        elf64_sym_t *symbol = (elf64_sym_t *) ((uintptr_t) symtab + i * symtab_hdr->sh_entsize);
        char *symname = &strtab[symbol->st_name];

        if (strcmp(symname, name)) {
            return symbol;
        }
    }

    return 0;
}

void elf64_load(void *binary)
{
    elf64_ehdr_t *ehdr = (elf64_ehdr_t *) binary;

    size_t i;
    for (i = 0; i < ehdr->e_phnum; ++i) {
        elf64_phdr_t *phdr = (elf64_phdr_t *) ((uintptr_t) binary + ehdr->e_phoff + i * ehdr->e_phsize);

        if (ELF_PT_LOAD != phdr->p_type)
            continue;

        uintptr_t source = (uintptr_t) binary + phdr->p_offset;
        uintptr_t target = (uintptr_t) heap_alloc(phdr->p_memsz);

        memcpy((void *) target, (void *) source, phdr->p_filesz);
        memset((void *) (target + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);

        size_t offset;
        for (offset = 0; offset < phdr->p_memsz; offset += 0x1000) {
            page_map(target + offset, phdr->p_vaddr + offset, PAGE_FLAG_WRITABLE | PAGE_FLAG_GLOBAL);
        }
    }
}
