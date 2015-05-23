OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain_bsp)
SECTIONS {
    .text 0xFFFFFF8000000000 : {
        *(.text)
        *(.rodata)
    }

    .data : {
        *(.data)
    }
    
    .bss : {
        *(.bss)
    }
    
    /DISCARD/ : {
        *(.comment)
    }
}