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

// Page Flags
#define PAGE_FLAG_PRESENT   (1 << 0)		//< entry is present
#define PAGE_FLAG_WRITABLE  (1 << 1)		//< page can be written to
#define PAGE_FLAG_USER      (1 << 2)		//< page can be accessed from DPL=3
#define PAGE_FLAG_GLOBAL    (1 << 8)		//< page sticks in TLB on CR3 writes

// Page Model Levels
#define PAGE_LEVEL_PML4     4
#define PAGE_LEVEL_PDP      3
#define PAGE_LEVEL_PD       2
#define PAGE_LEVEL_PT       1

// Entry analysis
#define PAGE_PHYSICAL(a)    (a & (0xFFFFFFFFFFFF << 12))
#define PAGE_INDEX(a,l)     ((a >> (12 + (l - 1) * 9)) & 0x1FF)

// Page structures
extern uint64_t page_pml4[512];
extern uint64_t page_idn_pdp[512];
extern uint64_t page_idn_pd[512 * 64];

/**
 * Maps the page at the given virtual address to the given physical one and
 * sets the provided flags.
 *
 * The required paging structures are allocated on the heap on demand.
 *
 * The physical and virtual address are aligned down to the lower page boundary.
 *
 * @param physical the physical address of the frame to map to
 * @param virtual the virtual address of the page to map
 * @param flags the flags to map with
 */
void page_map(uintptr_t physical, uintptr_t virtual, uint64_t flags);

/**
 * Invalidates a page in the CPU's TLB.
 *
 * The virtual address is aligned down to the lower page boundary.
 *
 * @param virtual the virtual address of the page to invalidate
 */
void page_invalidate(uintptr_t virtual);
