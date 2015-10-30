#pragma once

#include <handles.h>
#include <jegudiel.h>

#ifdef __cplusplus
    #include <terminals/base.hpp>
	#include <memory/manager_amd64.hpp>

    #define Handle Beelzebub::Handle
    #define TerminalBase Beelzebub::Terminals::TerminalBase
#else
    #define TerminalBase void
    //  Hue hue hue.
    //  Hue.
#endif

#define JG_INFO_ROOT_BASE          (0xFFFFFFFFFFD00000)

#define JG_INFO_OFFSET_EX(name)    ((uintptr_t)(JG_INFO_ROOT_BASE + JG_INFO_ROOT_EX->MCATS(name, _offset)))

#define JG_INFO_ROOT_EX            ((jg_info_root_t   *) JG_INFO_ROOT_BASE        )
#define JG_INFO_CPU_EX             ((jg_info_cpu_t    *) JG_INFO_OFFSET_EX(cpu   ))
#define JG_INFO_IOAPIC_EX          ((jg_info_ioapic_t *) JG_INFO_OFFSET_EX(ioapic))
#define JG_INFO_MMAP_EX            ((jg_info_mmap_t   *) JG_INFO_OFFSET_EX(mmap  ))
#define JG_INFO_MODULE_EX          ((jg_info_module_t *) JG_INFO_OFFSET_EX(module))
#define JG_INFO_STRING_EX          ((char             *) JG_INFO_OFFSET_EX(string))

__extern __cold __bland void kmain_bsp();
__extern __cold __bland void kmain_ap();

#ifdef __cplusplus
namespace Beelzebub
{
    extern Beelzebub::Memory::MemoryManagerAmd64 BootstrapMemoryManager;
}
#endif

__extern __noinline __bland Handle InitializeMemory();
__extern __noinline __bland Handle InitializeModules();
__extern __noinline __bland Handle InitializeInterrupts();

__extern __noinline __bland TerminalBase * InitializeTerminalProto();
__extern __noinline __bland TerminalBase * InitializeTerminalMain();

#ifdef __BEELZEBUB__TEST_MT
__extern __noinline __bland void StartMultitaskingTest();
#endif

#ifdef __cplusplus
    #undef Handle
    #undef TerminalBase
#endif
