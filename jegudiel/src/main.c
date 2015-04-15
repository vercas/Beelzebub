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

#include <acpi.h>
#include <elf64.h>
#include <gdt.h>
#include <heap.h>
#include <jegudiel.h>
#include <idt.h>
#include <info.h>
#include <ioapic.h>
#include <kernel.h>
#include <lapic.h>
#include <main.h>
#include <multiboot.h>
#include <pic.h>
#include <screen.h>
#include <smp.h>
#include <stdint.h>
#include <syscall.h>
#include <ports.h>

volatile uint8_t main_entry_barrier = 1;

void main_bsp(void)
{
	init_serial(COM1);
	screen_init();

	// Print header
	puts("Jegudiel v0.1\r\n");
	puts("Credits will be here eventually...\r\n");
	puts("----");

	puts("Loading IDT...\r\n");

	// Load the IDT
	idt_load(idt_address, IDT_LENGTH);
	idt_setup_loader();

	puts("Parsing Multiboot header...\r\n");

	// Initialize Jegudiel info tables and parse the multiboot tables
	info_init();
	multiboot_parse();

	puts("Initializing heap...\r\n");

	// Setup the heap
	heap_init();

	puts("Parsing ACPI...\r\n");

	// Now parse the ACPI tables and analyze the IO APICs
	acpi_parse();

	puts("Analyzing IOAPIC...\r\n");

	ioapic_analyze();

	puts("Deciphering kernel...");

	// Find, check and load the kernel binary
	puts(" FIND");    kernel_find();
	puts(" CHECK");   kernel_check();
	puts(" LOAD");    elf64_load(kernel_binary);
	puts(" ANALYZE"); kernel_analyze();

	puts("\r\n");

	puts("Initializing LAPIC...\r\n");

	// Initialize interrupt controllers
	lapic_detect();
	lapic_setup();
	ioapic_setup_loader();
	pic_setup();

	puts("Calibrating LAPIC timer...\r\n");

	// Calibrate the LAPIC timer
	lapic_timer_calibrate();

	puts("Booting application processors...\r\n");

	// Boot APs
	info_cpu[lapic_id()].flags |= JG_INFO_CPU_FLAG_BSP;
	smp_setup();

	puts("Setting up IDT and IOAPIC according to the kernel header...\r\n");

	// Setup IDT and IOAPIC according to kernel header
	idt_setup_kernel();
	ioapic_setup_kernel();

	puts("Setting up system call interface...\r\n");

	// Setup fast syscall support
	syscall_init();

	puts("Mapping structures...\r\n");

	// Setup mapping
	kernel_map_info();
	kernel_map_stack();
	kernel_map_idt();
	kernel_map_gdt();

	// Set free address
	info_root->free_paddr = (heap_top + 0xFFF) & ~0xFFF;

	/*screen_write("Video mode: ", 0, 23);
	screen_write_hex32(multiboot_info->vbe_mode, 12, 23);
	screen_write("; Info: ", 20, 23);
	screen_write_hex32(multiboot_info->vbe_mode_info, 28, 23);

	screen_write("Framebuffer type: ", 0, 22);
	screen_write_hex8(multiboot_info->framebuffer_type, 18, 22);
	screen_write("; W: ", 20, 22);
	screen_write_hex32(multiboot_info->framebuffer_width, 25, 22);
	screen_write("; H: ", 33, 22);
	screen_write_hex32(multiboot_info->framebuffer_height, 38, 22);
	screen_write("; P: ", 46, 22);
	screen_write_hex32(multiboot_info->framebuffer_pitch, 51, 22);
	screen_write("; BPP: ", 59, 22);
	screen_write_hex8(multiboot_info->framebuffer_bpp, 66, 22);

	screen_write("Address: ", 0, 21);
	screen_write_hex(multiboot_info->framebuffer_addr, 9, 21);
	screen_write("; Phys base: ", 25, 21);
	screen_write_hex32(VBE_MODE_INFO->physbase, 38, 21);*/

	puts("Jumping to kernel's boot entry...\r\n");

	// Lower main entry barrier and jump to the kernel entry point
	main_entry_barrier = 0;
	kernel_enter_bsp();
}

void main_ap(void)
{
	// Load the IDT
	idt_load((uintptr_t) &idt_data, IDT_LENGTH);

	// Enable LAPIC and calibrate the timer
	lapic_setup();
	lapic_timer_calibrate();

	// Setup stack mapping
	kernel_map_stack();

	// Setup fast syscall support
	syscall_init();

	// Signal complete AP startup
	++smp_ready_count;

	// Wait for main entry barrier, then enter the kernel (or halt)
	while (main_entry_barrier == 1);
	kernel_enter_ap();
}
