OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain_bsp)
SECTIONS
{
    . = 0xFFFFFFFF80000000;
    kernel_mapping_start = .;

    .text.hot ALIGN(16) : {
        *(.text.hot)
    }

    .text ALIGN(16) : {
        *(.text)
    }

    .text.unlikely : {
        *(.text.unlikely)
    }

    .text.assertions ALIGN(16) : {
        *(.text.assertions)
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

    .eh_frame           : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) }

    /* Exception handling  */
    .eh_frame           : ONLY_IF_RW { KEEP (*(.eh_frame)) *(.eh_frame.*) }
    .gnu_extab          : ONLY_IF_RW { *(.gnu_extab) }
    .gcc_except_table   : ONLY_IF_RW { *(.gcc_except_table .gcc_except_table.*) }
    .exception_ranges   : ONLY_IF_RW { *(.exception_ranges .exception_ranges*) }

    /* Thread Local Storage sections  */
    .tdata              : { *(.tdata .tdata.* .gnu.linkonce.td.*) }
    .tbss               : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) }

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

    .locks ALIGN(8) : {
        locks_section_start = .;
        *(.locks.chk)
        *(.locks.acq)
        *(.locks.rel)
        locks_section_end = .;
    }

    .thread_data ALIGN(16) : {
        thread_data_start = .;
        *(.bootstrap_thread)
        *(.thread_data)
        thread_data_end = .;
    }

    .process_data ALIGN(16) : {
        process_data_start = .;
        *(.bootstrap_process)
        *(.process_data)
        process_data_end = .;
    }

    .text.userland ALIGN(0x1000) : {
        userland_section_start = .;
        *(.text.userland)
        userland_section_end = .;
    }

    . = ALIGN(4096);
    kernel_mapping_end = .;

    /DISCARD/ : {
        *(.comment)
    }
}
