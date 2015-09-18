#include <jegudiel.h>
#include <system/isr.hpp>
#include <keyboard.hpp>
#include <entry.h>

#define FLAGS (                   \
	JG_HEADER_FLAG_IOAPIC_BSP   | \
	JG_HEADER_FLAG_X2APIC_ALLOW   \
)

__extern __used jg_header_root_t jegudiel_header = {
	JG_MAGIC,                               // magic
	FLAGS,                                  // flags
	0xFFFFFFFFFFF00000,                     // stack_vaddr
	JG_INFO_ROOT_BASE ,                     // info_vaddr
	0xFFFFFFFFFFCFF000,                     // idt_vaddr
	0xFFFFFFFFFFCFE000,                     // gdt_vaddr
	(uintptr_t)&kmain_ap,                   // ap_entry
	0,                                      // syscall_entry
	(uintptr_t)&IsrGates,                  // isr_entry_table
	{
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ0
		{0                      , KEYBOARD_IRQ_VECTOR    },   // IRQ1
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ2
		{0                      , 32                     },   // IRQ3 - COM2&4
		{0                      , 32                     },   // IRQ4 - COM1&3
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ5
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ6
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ7
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ8
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ9
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ10
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ11
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ12
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ13
		{JG_HEADER_IRQ_FLAG_MASK, 0                      },   // IRQ14
		{JG_HEADER_IRQ_FLAG_MASK, 0                      }    // IRQ15
	}
};
