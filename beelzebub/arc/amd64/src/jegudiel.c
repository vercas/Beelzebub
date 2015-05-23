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

#include <jegudiel.h>
#include <arc/isr.h>
#include <arc/keyboard.h>
#include <arc/entry.h>

#define FLAGS (                   \
	JG_HEADER_FLAG_IOAPIC_BSP   | \
	JG_HEADER_FLAG_X2APIC_ALLOW   \
)

jg_header_root_t jegudiel_header = {
	JG_MAGIC,                               // magic
	FLAGS,                                  // flags
	0xFFFFFFFFFFF00000,                     // stack_vaddr
	JG_INFO_ROOT_BASE ,                     // info_vaddr
	0xFFFFFFFFFFCFF000,                     // idt_vaddr
	0xFFFFFFFFFFCFE000,                     // gdt_vaddr
	(uintptr_t)&kmain_ap,                   // ap_entry
	0,                                      // syscall_entry
	(uintptr_t)&isr_gates,                  // isr_entry_table
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
