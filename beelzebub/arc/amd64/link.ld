OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain_bsp)
SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text.unlikely : {
        *(.text.unlikely)
    }

    .text ALIGN(16) : {
        *(.text)
    }

    .text.hot ALIGN(16) : {
        *(.text.hot)
    }

    .rodata ALIGN(0x1000) : {
        *(.rodata)
    }

    .rodata.cst8 ALIGN(8) : {
        *(.rodata.cst8)
    }

    .rodata.str1.8 ALIGN(8) : {
        *(.rodata.str1.8)
    }

    .rodata.str1.1 : {
        *(.rodata.str1.1)
    }

    .data ALIGN(0x1000) : {
        *(.data)
        . = ALIGN(16);
        *(.ap_bootstrap)
    }

    .data.tests ALIGN(32) : {
        tests_data_section_start = .;
        *(.data.tests)
        tests_data_section_end = .;
    }

    .bss ALIGN(64) : {
        *(.bss)
    }

    .text.startup ALIGN(0x1000) : {
        startup_section_start = .;
        *(.text.startup)
        startup_section_end = .;
    }

    .text.tests ALIGN(0x1000) : {
        tests_code_section_start = .;
        *(.text.tests)
        tests_code_section_end = .;
    }

    .text.userland ALIGN(0x1000) : {
        userland_section_start = .;
        *(.text.userland)
        userland_section_end = .;
    }

    . = ALIGN(4096);

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}
