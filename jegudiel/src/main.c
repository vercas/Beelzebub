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


    puts("[  ] Analyzing IOAPIC...");

    ioapic_analyze();

    puts("\r[OK]\r\n");


    puts("[  ] Deciphering kernel...");

    // Find, check and load the kernel binary
    puts(" FIND");    kernel_find();
    puts(" CHECK");   kernel_check();
    puts(" LOAD");    elf64_load(kernel_binary);
    puts(" ANALYZE"); kernel_analyze();

    puts("\r[OK]\r\n");


    puts("[  ] Initializing LAPIC...");

    // Initialize interrupt controllers
    lapic_detect();
    lapic_setup();
    ioapic_setup_loader();
    pic_setup();

    puts("\r[OK]\r\n");


    puts("[  ] Calibrating LAPIC timer...");

    // Calibrate the LAPIC timer
    lapic_timer_calibrate();

    puts("\r[OK]\r\n");


    puts("[  ] Booting application processors...");

    // Boot APs
    info_cpu[lapic_id()].flags |= JG_INFO_CPU_FLAG_BSP;
    smp_setup();

    puts("\r[OK]\r\n");


    puts("[  ] Setting up IDT and IOAPIC according to the kernel header...");

    // Setup IDT and IOAPIC according to kernel header
    idt_setup_kernel();
    ioapic_setup_kernel();

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
    puts("\r\nHanging!\r\n");

    for(;;);

    // Lower main entry barrier and jump to the kernel entry point
    //main_entry_barrier = 0;
    __atomic_clear(&main_entry_barrier, __ATOMIC_SEQ_CST);
    kernel_enter_bsp();
}

void main_ap(void)
{
    // Load the IDT
    idt_load((uintptr_t) &idt_data, IDT_LENGTH);

    puts("A");

    // Enable LAPIC and calibrate the timer
    lapic_setup();
    lapic_timer_calibrate();

    puts("B");

    // Setup stack mapping
    kernel_map_stack();

    puts("C");

    // Setup fast syscall support
    syscall_init();

    puts("D");

    // Signal complete AP startup
    //++smp_ready_count;
    uint64_t incrementor = 1; // Staying on the safe side.
    __atomic_add_fetch(&smp_ready_count, incrementor, __ATOMIC_SEQ_CST);

    puts("E");

    // Wait for main entry barrier, then enter the kernel (or halt)
    while (main_entry_barrier == 1);
    kernel_enter_ap();
}
