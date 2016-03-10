OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain_bsp)
SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text.unlikely : {
        *(.text.unlikely)
    }

    . = ALIGN(16);

    .text : {
        *(.text)
    }

    . = ALIGN(16);

    .text.hot : {
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
        . = ALIGN(16);
        *(.ap_bootstrap)
    }

    . = ALIGN(4096);
    
    .bss : {
        *(.bss)
    }
    
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }

    . = ALIGN(4096);
    startup_section_start = .;
    
    .text.startup : {
        *(.text.startup)
    }

    startup_section_end = .;
    . = ALIGN(4096);
}
