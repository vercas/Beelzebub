/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#include <acpi.h>
#include <elf64.h>
#include <gdt.h>
#include <heap.h>
#include <jegudiel.h>
#include <idt.h>
#include <info.h>
#include <kernel.h>
#include <main.h>
#include <multiboot.h>
#include <screen.h>
#include <stdint.h>
#include <syscall.h>
#include <ports.h>

volatile uint8_t main_entry_barrier = 1;

void main_bsp(void)
{
#ifdef __JEGUDIEL__SERIAL
    init_serial(COM1);
#endif
    screen_init();

    // Print header
    puts("Jegudiel v0.1 | https://github.com/vercas/Beelzebub/\r\n");
    puts("Copyright (c) 2015 by Lukas Heidemann and Alexandru-Mihai Maftei.\r\n");

    if (multiboot_info->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB)
        puts("VBE font is Free Pixel by levelb\r\n");

    puts("----");


    puts("[  ] Parsing Multiboot header...");

    // Initialize Jegudiel info tables and parse the multiboot tables
    info_init();
    multiboot_parse();

    puts("\r[OK]\r\n");


    if (multiboot_info->flags & (1 << 2))
    {
        puts("[  ] Parsing command-line arguments... ");

        //  Do shenanigans.
        puts((char *)(uintptr_t)multiboot_info->cmdline);

        puts("\r[OK]\r\n");
    }


    puts("[  ] Loading IDT...");

    // Load the IDT
    idt_load(idt_address, IDT_LENGTH);
    idt_setup_loader();

    puts("\r[OK]\r\n");


    puts("[  ] Initializing heap...");

    // Setup the heap
    heap_init();

    puts("\r[OK]\r\n");


    puts("[  ] Parsing ACPI...");

    // Now parse the ACPI tables and analyze the IO APICs
    acpi_parse();

    puts("\r[OK]\r\n");


    puts("[  ] Deciphering kernel...");

    // Find, check and load the kernel binary
    puts(" FIND");    kernel_find();
    puts(" CHECK");   kernel_check();
    puts(" LOAD");    elf64_load(kernel_binary);
    puts(" ANALYZE"); kernel_analyze();

    puts("\r[OK]\r\n");


    puts("[  ] Setting up IDT according to the kernel header...");

    idt_setup_kernel();

    puts("\r[OK]\r\n");


    puts("[  ] Setting up system call interface...");

    // Setup fast syscall support
    syscall_init();

    puts("\r[OK]\r\n");


    puts("[  ] Mapping structures...");

    // Setup mapping
    kernel_map_info();
    kernel_map_stack();
    kernel_map_idt();
    kernel_map_gdt();

    // Set free address
    info_root->free_paddr = (heap_top + 0xFFF) & ~0xFFF;

    // Reference multiboot header
    info_root->multiboot_paddr = (uint64_t)multiboot_info;

    puts("\r[OK]\r\n");


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

    puts("----");

    puts("Jumping to kernel's boot entry...\r\n");

    //for(;;);

    kernel_enter_bsp();
}
