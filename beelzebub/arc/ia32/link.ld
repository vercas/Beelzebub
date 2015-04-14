OUTPUT_FORMAT(elf32-x86)
ENTRY(kmain_bsp)
SECTIONS {
    .text 0xC0000000 : {
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