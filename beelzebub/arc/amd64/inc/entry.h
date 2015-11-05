#pragma once

#include <handles.h>
#include <jegudiel.h>

#ifdef __cplusplus
    #include <terminals/base.hpp>
    #include <system/cpuid.hpp>
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
    extern Memory::MemoryManagerAmd64 BootstrapMemoryManager;

    extern System::CpuId BootstrapCpuid;
}
#endif

__extern __cold __bland __noinline Handle InitializeInterrupts();
__extern __cold __bland __noinline Handle InitializePit();

// TODO: Don't depend on Jegudiel; let Jegudiel depend on Beelzebub!
__extern __cold __bland __noinline Handle InitializePhysicalAllocator(jg_info_mmap_t * map
					                                                , size_t cnt
					                                                , uintptr_t freeStart
					                                                , Domain * domain);
__extern __cold __bland __noinline Handle InitializePhysicalMemory();
__extern __cold __bland __noinline Handle InitializeVirtualMemory();

__extern __cold __bland __noinline Handle InitializeApic();

__extern __cold __bland __noinline Handle InitializeAcpiTables();
__extern __cold __bland __noinline Handle InitializeProcessingUnits();

__extern __cold __bland __noinline Handle InitializeModules();

__extern __cold __bland __noinline TerminalBase * InitializeTerminalProto();
__extern __cold __bland __noinline TerminalBase * InitializeTerminalMain();

#ifdef __BEELZEBUB__TEST_MT
__extern __cold __bland __noinline void StartMultitaskingTest();
#endif

#ifdef __cplusplus
    #undef Handle
    #undef TerminalBase
#endif
