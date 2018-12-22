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
#include <stdint.h>
#include <string.h>
#include <screen.h>

uintptr_t heap_top = 0;

/**
 * Sorts the modules in the info tables by their address in ascending order.
 *
 * This procedure is required in order to prevent clobbering the modules when
 * moving them to an area that is intersecting with the area the modules were
 * originally stored in.
 */
static void heap_modules_sort(void)
{
    size_t count = info_root->module_count;
    size_t i, j;

    if (count > 1)
    {
        for (i = 0; i < count - 1; ++i)
        {
            uintptr_t minAddr = info_module[i].address;
            size_t minInd = i;

            for (j = i + 1; j < count; ++j)
                if (info_module[j].address < minAddr)
                {
                    minInd = j;
                    minAddr = info_module[j].address;
                }

            if (i != minInd)
            {
                jg_info_module_t tmp = info_module[i];
                info_module[i] = info_module[minInd];
                info_module[minInd] = tmp;
            }
        }
    }
}

/**
 * Moves the modules to the end of the heap.
 */
static void heap_modules_move(void)
{
    size_t i;
    for (i = 0; i < info_root->module_count; ++i)
    {
        jg_info_module_t * mod = &info_module[i];

        void * buffer = heap_alloc(mod->length);

        memcpy((void *)buffer, (void *)(mod->address), mod->length);

        mod->address = (uintptr_t)buffer;
    }
}

void heap_init(void)
{
	extern uint8_t heap_mark;
	heap_top = (uintptr_t)(&heap_mark);

	heap_modules_sort();
    puts("sorted...");
	heap_modules_move();
}

void *heap_alloc(size_t size)
{
	size = (size + 0xFFFULL) & ~0xFFFULL;
	uintptr_t chunk = heap_top;
	heap_top += size;

	return (void *) chunk;
}

