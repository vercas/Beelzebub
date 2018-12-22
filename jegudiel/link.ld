OUTPUT_FORMAT(elf64-x86-64)
ENTRY(boot32_bsp)
SECTIONS
{
    . = 0x1000000;

    .text : {
        *(.multiboot)
        *(.text)
    }

    .data : {
        *(.data)
    }
    
    .info 0x1008000 : {
        info_root_data = .; . += 4096;
        info_mmap_data = .; . += 4096;
        info_module_data = .; . += 4096;
        info_strings_data = .; . += 4096;
    }

    .consts : {
        *(.rodata)
        *(.graphics)
    }
    
    .bss : {
        *(.bss)
    }
    
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }

    . = ALIGN(4096);
    heap_mark = .;
}