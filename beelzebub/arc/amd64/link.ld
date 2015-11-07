OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain_bsp)
SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text : {
        *(.ap_bootstrap)
        . = ALIGN(16);
        *(.text.startup)
        . = ALIGN(16);
        *(.text.unlikely)
        . = ALIGN(16);
        *(.text)
        . = ALIGN(16);
        *(.text.hot)
    }

    . = ALIGN(4096);

    .rodata : {
        *(.rodata.str1.8)
        *(.rodata.str1.1)
        . = ALIGN(64);
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
