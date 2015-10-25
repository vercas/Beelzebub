#pragma once

#include <metaprogramming.h>
#include <jegudiel.h>

#define JG_INFO_ROOT_BASE          (0xFFFFFFFFFFD00000)

#define JG_INFO_OFFSET_EX(name)    ((uintptr_t)(JG_INFO_ROOT_BASE + JG_INFO_ROOT_EX->MCATS(name, _offset)))

#define JG_INFO_ROOT_EX            ((jg_info_root_t   *) JG_INFO_ROOT_BASE        )
#define JG_INFO_CPU_EX             ((jg_info_cpu_t    *) JG_INFO_OFFSET_EX(cpu   ))
#define JG_INFO_IOAPIC_EX          ((jg_info_ioapic_t *) JG_INFO_OFFSET_EX(ioapic))
#define JG_INFO_MMAP_EX            ((jg_info_mmap_t   *) JG_INFO_OFFSET_EX(mmap  ))
#define JG_INFO_MODULE_EX          ((jg_info_module_t *) JG_INFO_OFFSET_EX(module))
#define JG_INFO_STRING_EX          ((char             *) JG_INFO_OFFSET_EX(string))

//__extern const size_t PageSize = 4 * 1024;
#define PAGE_SIZE ((size_t)4096)

__extern __cold __bland void kmain_bsp();
__extern __cold __bland void kmain_ap();
