; Dude I just had to fix 80-90% of your code.
; And I added quite a lot of my own.
; It's mine now. Shoo.

section .text
bits 32

global boot32_bsp
global boot32_ap

extern heap_top
extern multiboot_info
extern main_bsp
extern main_ap
extern gdt_data
extern gdt_pointer
extern page_pml4
extern page_idn_pdp
extern page_idn_pd

;   BSP entrypoint, supposed to be called by a Multiboot-compliant bootloader.
boot32_bsp:
    cli                                         ; Disable interrupts
    cld                                         ; Clear direction flag.

    mov     esp, boot32_stack_bsp               ; Grab a stack as quickly as possible.
    add     esp, 0x1000                         ; Don't forget that it 'grows' backwards.
    
    ;   Let's just test this...
    test    eax, 0x2BADB002                     ; Check the Multiboot magic number.
    jne     .multiboot_compliant                ; If it's cool, skip the panic.

    mov     edi, boot32_err_nomb                ; Pick what you're gonna scream.
    call    boot32_panic                        ; Scream.

.multiboot_compliant:
    mov     dword [multiboot_info], ebx         ; Preserve the pointer to the multiboot header.

    ;   Check for long mode support.
    mov     eax, 0x80000001                     ; This be the right value.
    cpuid                                       ; Do the deed.
    bt      edx, 29                             ; EDX bit 29
    jc      .long_mode_supported                ; Long mode supported

    mov     edi, boot32_err_nolm                ; Pick a message.
    call    boot32_panic                        ; Yell it out loud.

.long_mode_supported:
    ;   THESE FUNCTIONS HARDLY RESPECT THE ABI!

    ;call    boot32_com1_init                    ; Initialize the COM1 port ASAP.
    ;call    boot32_com2_init                    ; Also COM2.

    call    boot32_gdt                          ; Setup 64 bit GDT
    call    boot32_map                          ; Setup mapping

    ;mov     edi, boot32_msg_a
    ;call    boot32_com1_msg

    ;mov     esi, page_pml4
    ;mov     ecx, 4096 * 66
    ;call    boot32_com2_dump

    call    boot32_common                       ; Common shenanigans

    mov ax, 0x10                        ; Setup data segment selectors
    mov ds, ax
    mov ss, ax
    mov gs, ax
    mov fs, ax
    mov es, ax

    jmp 0x8:main_bsp                    ; Far jump

;   AP entry point, for protected mode, called by the 16-bit bootstrap code.
boot32_ap:
    cli                                         ; Clear interrupts

    mov         esp, 0x1000                     ; Stack size
    lock xadd   dword [heap_top], esp           ; Allocate stack from heap
    add         esp, 0x1000                     ; EDX now contains the lower address of the stack; this changes EDX to the higher address ("bottom")
    ;mov         esp, edx                        ; Assign the stack.

    call        boot32_common                   ; Common shenanigans
    jmp         0x8:main_ap                     ; Jump to the right code segment.

; Common initialization for both, the BSP and the APs.
boot32_common:
    ;   Enabling PAE.
    mov     eax, cr4                            ; Load CR4 (Control Register 4).
    bts     eax, 5                              ; Set bit 5.
    mov     cr4, eax                            ; Set CR4.

    ;   Die in a grease fire, A20 line.

    mov ecx, 0xC0000080                 ; Enable LM in MSR
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, gdt_pointer                ; Load the 64 bit GDT pointer
    lgdt [eax]                          ; Load the 64 bit GDT

    mov eax, page_pml4                  ; Load PML4 address
    mov cr3, eax                        ; Load the PML4

    mov eax, cr0                        ; Enable paging
    or eax, 1 << 31
    mov cr0, eax
    ret

;   Sets up 64 bit GDT based on the template.
boot32_gdt:
    mov     edi, gdt_data                       ; Destination GDT base
    mov     esi, boot32_gdt_data                ; Source GDT base
    mov     ecx, boot32_gdt_data.end            ; Source GDT end
    sub     ecx, esi                            ; Subtract base from end and you obtain the length!

    rep movsb                                   ; Copy the GDT!

    ret                                         ; Done!

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

.boot32_com1_msg_check:
    ; CHECK FIFO
    mov     dx, 0x3F8 + 5                       ; This is where I check if the buffer's empty.
    in      al, dx                              ; Grab the data...

    bt      ax, 5                               ; If the bit's clear (buffer non-empty)
    jnc     .boot32_com1_msg_check              ; Try again!

    mov     dx, 0x3F8                           ; Set the correct port for output.
    out     dx, al                              ; Output to COM1.

    jmp .boot32_com1_msg_next_byte              ; Continue to the next character.

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

    jmp .boot32_com2_dump_next_byte             ; Continue to the next character.

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
    jmp .boot32_msg_next_byte                   ; Continue to the next character.

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

boot32_msg_a db "Point A", 0
boot32_msg_b db "Point B", 0
boot32_msg_c db "Point C", 0
boot32_msg_d db "Point D", 0
boot32_msg_e db "Point E", 0
boot32_msg_f db "Point F", 0

boot32_gdt_data:
    .null:
        dd 0x0
        dd 0x0
    .kernel_code:
        dd 0x0
        dd 0x209800
    .kernel_data:
        dd 0x0
        dd 0x209200
    .user_code:
        dd 0x0
        dd 0x20F800
    .user_data:
        dd 0x0
        dd 0x20F200
    .end:

section .bss

align 4096
boot32_stack_bsp: resb 0x1000
