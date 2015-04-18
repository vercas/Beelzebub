#pragma once

#include <metaprogramming.h>
#include <jegudiel.h>

const size_t PageSize = 4 * 1024;

__extern __bland void kmain_bsp();
__extern __bland void kmain_ap();

__bland void InitializeMemory(jg_info_mmap_t * map, uint32_t cnt, uintptr_t freeStart);

