OUTPUT_FORMAT(elf64-x86-64)
ENTRY(boot32_bsp)
SECTIONS {
    .text 0x100000 : {
        *(.multiboot)
        *(.text)
        *(.rodata)
    }

    .data : {
        *(.data)
    }
    
    .bss : {
        *(.bss)
    }
    
    .info 0x108000 : {
        idt_data = .; . += 4096;
        gdt_data = .; . += 4096;
        page_pml4 = .; . += 4096;
        page_idn_pdp = .; . += 4096;
        page_idn_pd = .; . += 4096 * 64;
        info_root_data = .; . += 4096;
        info_mmap_data = .; . += 4096;
        info_module_data = .; . += 4096;
        info_ioapic_data = .; . += 4096;
        info_strings_data = .; . += 4096;
    } 
    
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }

    . = ALIGN(4096);
    heap_mark = .;
}