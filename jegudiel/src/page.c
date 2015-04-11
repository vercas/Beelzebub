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

#include <heap.h>
#include <info.h>
#include <page.h>
#include <stdint.h>
#include <string.h>

static uint64_t *page_struct_get(uintptr_t, uint8_t, bool);
static uint64_t *page_entry_get(uintptr_t, uint8_t, bool);

/**
 * Returns the physical address of the currently active PML4.
 *
 * @return physical address of current PML4
 */
static uint64_t page_pml4_get(void)
{
	uint64_t pml4;
	asm volatile ("mov %%cr3, %0" : "=a" (pml4));
	return pml4;
}

/**
 * Returns the page structure of level <level> that covers the virtual
 * address <virtual>.
 *
 * If <create> is true, the structure and all higher level ones will be
 * created, if they do not exist. Otherwise a null pointer is returned,
 * if one of them does not exist.
 *
 * @param virtual The virtual address covered by the structure.
 * @param level The level of the structure to return.
 * @param create Whether to create the structure and higher level structures.
 * @return pointer to the structure or null pointer if <create> is false and
 *  there is no such structure
 */
static uint64_t *page_struct_get(uintptr_t virtual, uint8_t level, bool create)
{
	if (PAGE_LEVEL_PML4 == level)
		return (uint64_t *) page_pml4_get();

	uint64_t *parent_entry = page_entry_get(virtual, level + 1, create);

	if (0 == parent_entry)
		return 0;

	if (0 == (*parent_entry & PAGE_FLAG_PRESENT)) {
		if (create) {
			uintptr_t frame = (uintptr_t) heap_alloc(0x1000);
			uint64_t flags = PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE | PAGE_FLAG_USER;

			*parent_entry = frame | flags;

			memset((void *) frame, 0, 0x1000);

		} else {
			return 0;
		}
	}

	return (uint64_t *) PAGE_PHYSICAL(*parent_entry);
}

/**
 * Returns the entry in the page structure of level <level> that covers the
 * virtual address <virt>.
 *
 * See page_struct_get for <create>.
 *
 * @param virt The virtual address covered by the entry.
 * @param level The level of the structure the entry is stored in.
 * @param create Whether to create the containing structure and the higher level ones.
 * @return pointer to entry in a page structure or null pointer if <create> is false
 *  and there is no such structure
 */
static uint64_t *page_entry_get(uintptr_t virtual, uint8_t level, bool create)
{
	uint64_t *pstruct = page_struct_get(virtual, level, create);

	if (0 == pstruct)
		return 0;

	return &pstruct[PAGE_INDEX(virtual, level)];
}

void page_map(uintptr_t physical, uintptr_t virtual, uint64_t flags)
{
	physical &= ~0xFFF;
	virtual &= ~0xFFF;

	uint64_t *pte = page_entry_get(virtual, PAGE_LEVEL_PT, true);
	*pte = PAGE_FLAG_PRESENT | flags | physical;

	page_invalidate(virtual);
}

void page_invalidate(uintptr_t virtual)
{
	virtual &= ~0xFFF;
	asm volatile ("invlpg %0" :: "m" (virtual));
}
