OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain_bsp)
SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text : {
        *(.text)
    }

    . = ALIGN(4096);

    .rodata : {
        *(.rodata)
    }

    . = ALIGN(4096);

    .data : {
        *(.data)
    }

    . = ALIGN(4096);
    
    .bss : {
        *(.bss)
    }
    
    /DISCARD/ : {
        *(.comment)
    }

    . = ALIGN(4096);
}
