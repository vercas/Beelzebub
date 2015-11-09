; Dude I just had to fix 80-90% of your code.
; And I added quite a lot of my own.
; It's mine now. Shoo.

extern heap_top
extern multiboot_info
extern main_bsp
extern main_ap
extern gdt_pointer

global gdt_data
global idt_data

global page_pml4
global page_idn_pdp
global page_idn_pd

global boot32_bsp
global boot32_stack_bsp

section .text

bits 32

;   BSP entrypoint, supposed to be called by a Multiboot-compliant bootloader.
boot32_bsp:
    cli
    cld
    ;   This ain't gettin' interrupted! Also, the direction flag is cleared for
    ;   the sake of building the paging tables.

    mov     esp, boot32_stack_bsp
    add     esp, 0x3000
    ;   A stack is always handy.
    
    test    eax, 0x2BADB002
    jne     .multiboot_compliant
    ;   Multiboot says check, so the bootloader checks.

    mov     edi, boot32_err_nomb
    call    boot32_panic
    ;   Let the dear user know.

.multiboot_compliant:

    mov     dword [multiboot_info], ebx
    ;   This will be required later.

    mov     eax, 0x80000001
    cpuid
    mov     ebp, edx
    ;   We will require several bits from this leaf.

    bt      ebp, 29
    jc      .long_mode_supported
    ;   Check for long mode support and proceed if found.

    mov     edi, boot32_err_nolm
    call    boot32_panic
    ;   The bootloader is for x64 only, at least right now.

.long_mode_supported:

    mov     edi, gdt_data
    mov     esi, boot32_gdt_data
    mov     ecx, boot32_gdt_data.end
    sub     ecx, esi
    rep movsb
    ;   Copy the GDT entries from template into real GDT.

    mov     ecx, edi
    and     ecx, 0xFFF
    neg     ecx
    add     ecx, 0x1000
    ;   ecx = 0x1000 - (&GDT & 0xFFF)!
    ;   AKA rest of bytes on the GDT's page.

    mov     eax, 0
    rep stosb
    ;   Clear the rest of the GDT.

    call    boot32_map                          ; Setup mapping

    mov     eax, cr4
    bts     eax, 5
    ;   Enable PAE.

    bt      ebp, 26
    jnc     .write_cr4
    bts     eax, 7
    ;   And PGE (global pages) if available.

.write_cr4:
    mov     cr4, eax
    ;   And flush.

    mov     eax, page_pml4
    mov     cr3, eax
    ;   Load PML4.

    mov     ecx, 0xC0000080
    rdmsr
    bts     eax, 8
    ;   Enable long mode

    bt      ebp, 20
    jnc     .write_efer
    bts     eax, 11
    ;   And NX/XD bit if available.

.write_efer:
    wrmsr
    ;   Writes the value to IA32_EFER

    mov     eax, cr0
    bts     eax, 31
    mov     cr0, eax
    ;   Enable paging.

    lgdt    [gdt_pointer]
    ;   Load the 64-bit GDT.

    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    ;   Make sure the data segments are correct.

    jmp     0x8:main_bsp
    ;   Far jump into long mode! :D

;   Sets up identity mapping.
boot32_map:
    ;   First, add the PDPT as a PML4e.
    mov     edi, page_pml4                      ; Base address of PML4.
    mov     eax, page_idn_pdp                   ; PDPT address.
    or      eax, 3                              ; Set it as present and writable.
    stosd                                       ; First entry in PML4.
    add     edi, 4                              ; Align. This will help mapping with 1-GiB pages.

    ;   Secondly, check for 1-GiB pages.
    mov     eax, 0x80000001                     ; This be the right value...
    cpuid                                       ; For CPUID...
    bt      edx, 26                             ; EDX bit 26.
    jc      .map_1gib                           ; Jump on carry (set).

;   2 mebibyte pages!
    
    ;   The PDs ought to be added as PDPTes.
    mov     edi, page_idn_pdp                   ; Base address of PDPT
    mov     eax, page_idn_pd                    ; Base address of PDs.
    or      eax, 3                              ; Present and writable.
    mov     ecx, 64                             ; 64 entries (PDs)

.map_2mib_pdptes_loop:
    ;   Add the PDPTe.
    stosd                                       ; Store entry.
    add     edi, 4                              ; Align

    add     eax, 0x1000                         ; Advance by one page

    loop    .map_2mib_pdptes_loop               ; If it didn't just add the 64th PD as PDPTe, continue.

.map_2mib_pdes:
    ;   Finally, the PD entries!
    mov     edi, page_idn_pd                    ; Destination index = PDPT
    mov     eax, 0b10000011                     ; EAX starts with the flags: present, writable, large (1-GiB page)
    xor     ebx, ebx                            ; Used as a null register.
    xor     edx, edx                            ; Used to extend EAX.

    mov     ecx, 512 * 64                       ; 512 entries per PD for 64 PDs.

.map_2mib_pdes_loop:
    ;   One entry at a time.
    stosd   ;   *(edi += 4) = eax; (low dword)
    xchg    eax, edx
    stosd   ;   *(edi += 4) = edx; (high dword)
    xchg    eax, edx

    add     eax, 0x200000                       ; Add 2 MiB.
    adc     edx, ebx                            ; Add 0 + carry bit of previous addition.

    loop    .map_2mib_pdes_loop                 ; If it didn't just add the 512th PDPTe to the 64th PD, continue.

    ret     ; Done.

;   1 gibibyte pages!

.map_1gib:
    ;   Now, the PDs are recycled and used as PDPTs.
    ; EDI has been set before and it should point to the second entry of the PML4.
    mov     eax, page_idn_pd                    ; Base address of PDs to recycle.
    or      eax, 3                              ; Set as present and writable.
    mov     ecx, 64                             ; Number of PDPTs (PDs) left to add.

.map_1gib_pds_loop:
    ;   Add this PDPT.
    stosd                                       ; Store enrty.
    add     edi, 4                              ; Align.

    add     eax, 0x1000                         ; Set EAX to point to the next PD.
    
    loop    .map_1gib_pds_loop                  ; If it didn't just add the 64th PD as PML4e, continue.

.map_1gib_pdptes:
    ;   Finally, the PDPT entries!
    mov     edi, page_idn_pdp                   ; Destination index = PDPT
    mov     eax, 0b10000011                     ; EAX starts with the flags: present, writable, large (1-GiB page)
    xor     ebx, ebx                            ; Used as a null register.
    xor     edx, edx                            ; Used to extend EAX.

    mov     ecx, 512                            ; 512 entries.

.map_1gib_pdptes_loop:
    ;   One entry at a time.
    stosd   ;   *(edi += 4) = eax; (low dword)
    xchg    eax, edx
    stosd   ;   *(edi += 4) = edx; (high dword)
    xchg    eax, edx

    add     eax, 0x40000000                     ; Add 1 GiB.
    adc     edx, ebx                            ; Add 0 + carry bit of previous addition.

    loop    .map_1gib_pdptes_loop               ; If it didn't just add the 512th PDPTe, continue.

.map_1gib_pdes:
    ;   But now, the PD entries, which are recycled as PDPTs.
    mov     edi, page_idn_pd                    ; Destination index = first PD
    ;   EAX and EDX will continue where they left off.

    mov     ecx, 512 * 64                       ; 512 entries per table for 64 tables!

.map_1gib_pdes_loop:
    ;   One entry at a time.
    stosd   ;   *(edi += 4) = eax; (low dword)
    xchg    eax, edx
    stosd   ;   *(edi += 4) = edx; (high dword)
    xchg    eax, edx

    add     eax, 0x40000000                     ; Add 1 GiB.
    adc     edx, ebx                            ; Add 0 + carry bit of previous addition.

    loop    .map_1gib_pdes_loop                 ; If it didn't just add the 512th PDPTe to the 64th PD, continue.

    ret     ; Done.


;   Prepares the COM1 serial port for output.
boot32_com1_init:
    push    ebp

    push    eax
    push    edx

    ;   Disable interrupts.
    mov     dx, 0x3F8 + 1
    mov     al, 0x00
    out     dx, al

    ;   Enable DLAB, setting baud rate divisor.
    mov     dx, 0x3F8 + 3
    mov     al, 0x80
    out     dx, al

    ;   Divisor will be 3. (low byte)
    mov     dx, 0x3F8 + 0
    mov     al, 0x03
    out     dx, al

    ;   (high byte)
    mov     dx, 0x3F8 + 1
    mov     al, 0x00
    out     dx, al

    ;   8 bits, no parity, one stop bit.
    mov     dx, 0x3F8 + 3
    mov     al, 0x03
    out     dx, al

    ;   Enable FIFO, clear, and 14-byte buffer.
    mov     dx, 0x3F8 + 2
    mov     al, 0xC7
    out     dx, al

    ;   And finally enable IRQs and set RTS/DSR.
    mov     dx, 0x3F8 + 4
    mov     al, 0x0B
    out     dx, al

    pop     edx
    pop     eax

    pop     ebp
    ret


; Writes a message to the COM1 serial port.
;
; Parameters:
;   ESI - address of null-terminated string.
boot32_com1_msg:
    push    ebp                                 ; Preserve EBP... For whatever reason.

    push    edx                                 ; Preserve EDX.
    push    eax                                 ; Preserve EAX.

.boot32_com1_msg_next_byte:
    lodsb                                       ; Load character from string

    cmp     al, 0                               ; If null...
    je      .boot32_com1_msg_end                ; End.

    mov     dx, 0x3F8 + 5                       ; This is where I check if the buffer's empty.

.boot32_com1_msg_check:
    ; CHECK FIFO
    in      al, dx                              ; Grab the data...

    bt      ax, 5                               ; If the bit's clear (buffer non-empty)
    jnc     .boot32_com1_msg_check              ; Try again!

    mov     dx, 0x3F8                           ; Set the correct port for output.
    out     dx, al                              ; Output to COM1.

    jmp     .boot32_com1_msg_next_byte          ; Continue to the next character.

.boot32_com1_msg_end:
    pop     eax                                 ; Restore EAX.
    pop     edx                                 ; Restore EDI.

    pop     ebp                                 ; And restore EBP.
    ret                                         ; Done.


;   Prepares the COM2 serial port for output.
boot32_com2_init:
    push    ebp

    push    eax
    push    edx

    ;   Disable interrupts.
    mov     dx, 0x2F8 + 1
    mov     al, 0x00
    out     dx, al

    ;   Enable DLAB, setting baud rate divisor.
    mov     dx, 0x2F8 + 3
    mov     al, 0x80
    out     dx, al

    ;   Divisor will be 3. (low byte)
    mov     dx, 0x2F8 + 0
    mov     al, 0x03
    out     dx, al

    ;   (high byte)
    mov     dx, 0x2F8 + 1
    mov     al, 0x00
    out     dx, al

    ;   8 bits, no parity, one stop bit.
    mov     dx, 0x2F8 + 3
    mov     al, 0x03
    out     dx, al

    ;   Enable FIFO, clear, and 14-byte buffer.
    mov     dx, 0x2F8 + 2
    mov     al, 0xC7
    out     dx, al

    ;   And finally enable IRQs and set RTS/DSR.
    mov     dx, 0x2F8 + 4
    mov     al, 0x0B
    out     dx, al

    pop     edx
    pop     eax

    pop     ebp
    ret


; Dumps memory to the COM2 serial port.
;
; Parameters:
;   ESI - address of memory.
;   ECX - number of bytes.
boot32_com2_dump:
    push    ebp                                 ; Preserve EBP... For whatever reason.

    push    edx                                 ; Preserve EDX.
    push    ebx                                 ; Preserve EBX.
    push    eax                                 ; Preserve EAX.

    mov     ebx, ecx                            ; EBX will now contain the number of bytes.

.boot32_com2_dump_next_byte:
    cmp     ebx, 0                              ; If the are no more bytes to dump...
    jle     .boot32_com2_dump_end               ; End.

.boot32_com2_dump_check:
    ; CHECK FIFO
    mov     dx, 0x2F8 + 5                       ; This is where I check if the buffer's empty.
    in      al, dx                              ; Grab the data...

    bt      ax, 5                               ; If the bit's clear (buffer non-empty)
    jnc     .boot32_com2_dump_check             ; Try again!

    ;   Dump no more than 14 bytes!
    mov     ecx, 14                             ; Max 14 bytes.
    cmp     ecx, ebx                            ; Compare ECX with EBX.
    jle     .boot32_com2_dump_write             ; If ECX < EBX then skip the next statement
    mov     ecx, ebx                            ; ECX will not me larger than EBX this way.

.boot32_com2_dump_write:
    sub     ebx, ecx                            ; Subtract the number of bytes to be written from the number of bytes left.

    mov         dx, 0x2F8                       ; Set the correct port for output.
    rep outsb                                   ; Output ECX bytes to COM2.

    jmp     .boot32_com2_dump_next_byte         ; Continue to the next character.

.boot32_com2_dump_end:
    pop     eax                                 ; Restore EAX.
    pop     ebx                                 ; Restore EBX.
    pop     edx                                 ; Restore EDI.

    pop     ebp                                 ; And restore EBP.
    ret                                         ; Done.


; Writes a message to the screen.
; ONLY WORKS WITH VGA TEXT RIGHT NOW!
;
; Parameters:
;   ESI - address of null-terminated string.
boot32_msg:
    push    ebp                                 ; Preserve EBP... For whatever reason.

    push    edi                                 ; Preserve EDI.
    push    eax                                 ; Preserve EAX.

    mov     edi, 0xb80A0                        ; Write to VGA video memory

.boot32_msg_next_byte:
    lodsb                                       ; Load character from string

    cmp     al, 0                               ; If null...
    je      .boot32_msg_end                     ; End.

    mov     ah, 0xF0                            ; Black on white

    stosw                                       ; Write to video memory.
    jmp     .boot32_msg_next_byte               ; Continue to the next character.

.boot32_msg_end:
    pop     eax                                 ; Restore EAX.
    pop     edi                                 ; Restore EDI.

    pop     ebp                                 ; And restore EBP.
    ret                                         ; Done.

; Writes a message to the screen, then enters an infinite loop.
; ONLY WORKS WITH VGA TEXT RIGHT NOW!
;
; Parameters:
;   ESI - address of null-terminated string.
boot32_panic:
    mov edi, 0xb8000                       ; Write to VGA video memory
.boot32_panic_next_byte:
    lodsb                                  ; Load character
    cmp al, 0                              ; Null?
    je .boot32_panic_end
    or ax, 0x00F0                          ; Black on white
    stosb                                  ; Write to screen
    jmp .boot32_panic_next_byte                         ; Continue with next byte

.boot32_panic_end:
    hlt
    jmp     .boot32_panic_end                                ; Infinite loop

section .rodata

boot32_err_nomb: db "Must be launched by a Multiboot-compliant bootloader.", 0
boot32_err_nolm: db "64 bit long mode not supported on this CPU.", 0

;boot32_msg_a db "Point A", 0
;boot32_msg_b db "Point B", 0
;boot32_msg_c db "Point C", 0
;boot32_msg_d db "Point D", 0
;boot32_msg_e db "Point E", 0
;boot32_msg_f db "Point F", 0

boot32_gdt_data:
.null:          dq 0x0000000000000000
.kernel_code:   dq 0x0020980000000000   ;   64-bit kernel code
.kernel_data:   dq 0x0020920000000000   ;   64-bit kernel data..?
.user_code_32:  dq 0x0040FA0000000000   ;   32-bit user code
.user_data_32:  dq 0x0040F20000000000   ;   I think this is 32-bit user data...
.user_code:     dq 0x0020F80000000000   ;   64-bit user code
.user_data:     dq 0x0020F20000000000   ;   64-bit user data..?
.end:           db 0 ; dummy

;dd 0, 0
;dd 0x00000000, 0x00209A00   ; 0x08: 64-bit Code
;dd 0x00000000, 0x00009200   ; 0x10: 64-bit Data
;dd 0x00000000, 0x0040FA00   ; 0x18: 32-bit User Code
;dd 0x00000000, 0x0040F200   ; 0x20: User Data
;dd 0x00000000, 0x0020FA00   ; 0x28: 64-bit User Code
;dd 0x00000000, 0x0000F200   ; 0x30: User Data (64 version)

section .bss

align 4096

boot32_stack_bsp: resb 4096 * 3

page_pml4: resb 4096
page_idn_pdp: resb 4096
page_idn_pd: resb 4096 * 64

idt_data: resb 4096
gdt_data: resb 4096
