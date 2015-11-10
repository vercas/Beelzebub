#include <jegudiel.h>
#include <system/isr.hpp>
#include <keyboard.hpp>
#include <entry.h>

__extern
{
    __used jg_header_root_t jegudiel_header = {
        JG_MAGIC,                               // magic
        0,                                      // flags
        0xFFFFFFFFFFFFC000,                     // stack_vaddr
        JG_INFO_ROOT_BASE ,                     // info_vaddr
        0xFFFFFFFFFFFFA000,                     // idt_vaddr
        0xFFFFFFFFFFFF2000,                     // gdt_vaddr
        (uintptr_t)&kmain_ap,                   // ap_entry
        0,                                      // syscall_entry
        (uintptr_t)&IsrGates                    // isr_entry_table
    };
}
