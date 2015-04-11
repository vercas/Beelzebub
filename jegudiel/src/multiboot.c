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

#include <info.h>
#include <multiboot.h>
#include <stdint.h>
#include <string.h>

multiboot_info_t *multiboot_info;

/**
 * Aligns a memory map entry to page boundaries.
 * 
 * Always aligns available entries to the smaller boundaries, that is increasing
 * the address and decreasing the length and unavailable to the larger.
 * 
 * @param entry the entry to align
 */
static void multiboot_align_mmap(jg_info_mmap_t *entry)
{
    uintptr_t aligned_addr;
    
    if (entry->available) {
        aligned_addr = (entry->address + 0xFFF) & ~0xFFF;
        
        entry->length -= (entry->address - aligned_addr);
        entry->length &= ~0xFFF;
        
    } else {
        aligned_addr = entry->address & ~0xFFF;
        
        entry->length += (aligned_addr - entry->address);
        entry->length = (entry->length + 0xFFF) & ~0xFFF;
    }
    
    entry->address = aligned_addr;
}

static void multiboot_parse_mmap(multiboot_mmap_t *mmap, size_t length)
{
    while (0 != length) {
        jg_info_mmap_t *jgentry = &info_mmap[info_root->mmap_count++];
        
        jgentry->address = mmap->address;
        jgentry->length = mmap->length;
        jgentry->available = mmap->type == 1;
        
        multiboot_align_mmap(jgentry);
        
        length -= mmap->size + sizeof(uint32_t);
        mmap = (multiboot_mmap_t *) ((uintptr_t) mmap + mmap->size + sizeof(uint32_t));
    }
}

static void multiboot_parse_mods(multiboot_mod_t *mods, size_t count)
{
    size_t i;
    
    for (i = 0; i < count; ++i) {
        multiboot_mod_t *mod = &mods[i];
        
        jg_info_module_t *jgmod = &info_module[info_root->module_count++];
        jgmod->address = mod->start;
        jgmod->length = mod->end - mod->start;
        
        char *name = (char *) (uintptr_t) mod->cmdline;
        size_t name_len = strlen(name);
        char *name_tbl = info_string_alloc(name_len);
        memcpy(name_tbl, name, name_len);
        
        jgmod->name = ((uintptr_t) name_tbl - (uintptr_t) info_strings);
    }
}

void multiboot_parse()
{
    multiboot_mmap_t *mmap = (multiboot_mmap_t *) (uintptr_t) multiboot_info->mmap_addr;
    multiboot_mod_t *mods = (multiboot_mod_t *) (uintptr_t) multiboot_info->mods_addr;
    
    multiboot_parse_mmap(mmap, multiboot_info->mmap_length);
    multiboot_parse_mods(mods, multiboot_info->mods_count);
}
