;   This file attempts to make a switch from real mode to long mode directly.
;
;

extern kmain_ap

extern KernelGdtPointer

extern ApStackTopPointer
extern ApInitializationLock

global ApBootstrapBegin
global ApBootstrapEnd
global BootstrapPml4Address

;   The physical location of this code.
%define PHYSADDR 0x1000

;   This will be used by the linker, mainly.
section .ap_bootstrap

bits 16     ;   Start small.

align 16    ;   Make sure there is some alignment going.

;   Marks the beginning of the AP bootstrap code.
ApBootstrapBegin:
    cli
    ;   Make sure the core doesn't get interrupted while doing its business.

    jmp 0x0:.realMode - ApBootstrapBegin + PHYSADDR
    ;   Clear code segment by jumping to absolute address in segment 0.

.realMode:
    mov     esp, ApTempStack.end - ApBootstrapBegin + PHYSADDR
    ;   Get a basic little stack working.

    mov     eax, cr4
    bts     eax, 5
    bts     eax, 7
    mov     cr4, eax
    ;   Enable PAE and PGE (global pages).

    mov     eax, [BootstrapPml4Address - ApBootstrapBegin + PHYSADDR]
    mov     cr3, eax
    ;   Load the PML4.

    mov     ecx, 0xC0000080
    rdmsr
    bts     eax, 8
    wrmsr
    ;   Enable long mode.

    mov     eax, cr0
    bts     eax, 0
    bts     eax, 31
    mov     cr0, eax
    ;   Protected mode and paging.

    mov     al, 'C'
    out     dx, al

    lgdt    [cs:ApBootstrapGdt64R - ApBootstrapBegin + PHYSADDR]
    ;   Load a GDT which contains long mode segments.

    mov     dx, 0x3F8
    mov     al, 'A'
    out     dx, al

    jmp     0x08:.long - ApBootstrapBegin + PHYSADDR
    ;   Long jump into protected mode!

bits 64 ;   Then long mode directly.

.long:
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    ;   Make sure the data segments are correct.
    ;   The following code involves data fetching, so better safe than sorry?

;     mov     al, 'B'
;     out     dx, al

;     mov     ecx, esp

;     mov     rax, KernelGdtPointer
;     push    rax
;     push    qword [rax]
;     mov     rax, ApStackTopPointer
;     push    rax
;     push    qword [rax]
;     mov     rax, ApInitializationLock
;     push    rax
;     mov     eax, dword [rax]
;     push    rax

;     mov     esi, esp
;     sub     ecx, esp

;     push    rsi
;     push    rcx

;     add     rcx, 16
;     sub     rsi, 16

;     mov     al, 'C'
;     out     dx, al

;     mov     rax, Com2Dump
;     jmp     rax
; PostDump:

    mov     al, 'D'
    out     dx, al

    mov     rax, KernelGdtPointer
    lgdt    [rax]
    ;   Load the GDT for domain 0.

    mov     al, 'E'
    out     dx, al

    mov     rax, ApStackTopPointer
    mov     rsp, qword [rax]
    ;   The entry point will surely need a stack.

    mov     al, 'F'
    out     dx, al

    mov     rax, ApInitializationLock
    mov     dword [rax], 0
    ;   Tell the BSP that initialization is complete.

    mov     al, 'G'
    out     dx, al

    mov     rax, kmain_ap
    jmp     rax
    ;   Finally, jump to kernel.

    hlt
    ;   Eh, just makin' sure?

;   And now, data.

align 16    ;   Make it all tidy.

ApTempStack:
    times 128 db 0
.end:
    db 0 ;  DUMMY

align 16

ApBootstrapGdt64R:
    .Limit: dw 0x17
    .Base: dd ApBootstrapGdt64 - ApBootstrapBegin + PHYSADDR

align 8

ApBootstrapGdt64:
    .null:
        dq 0x0000000000000000
    .kernel_code:       ;   64-bit kernel code
        dq 0x0020980000000000
    .kernel_data:       ;   64-bit kernel data
        dq 0x0020920000000000
;   This is a trimmed down version of the GDT probided by Jegudiel.

align 16

BootstrapPml4Address:
    dq 0

align 16    ;   Just make sure things are neat and aligned.

;   Marks the end of the AP bootstrap code.
ApBootstrapEnd:

db 0
;   Dummy.

str_rlm: db "Reached long mode!", 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   The following code is for debugging!   ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Writes a message to the COM1 serial port.
;
; Parameters:
;   ESI - address of null-terminated string.
Com1Msg:
    ;push    ebp                                 ; Preserve EBP... For whatever reason.

    ;push    edx                                 ; Preserve EDX.
    ;push    eax                                 ; Preserve EAX.

.next_byte:
    lodsb                                       ; Load character from string

    cmp     al, 0                               ; If null...
    je      .end                                ; End.

    mov     dx, 0x3F8 + 5                       ; This is where I check if the buffer's empty.

.check:
    ; CHECK FIFO
    in      al, dx                              ; Grab the data...

    bt      ax, 5                               ; If the bit's clear (buffer non-empty)
    jnc     .check                              ; Try again!

    mov     dx, 0x3F8                           ; Set the correct port for output.
    out     dx, al                              ; Output to COM1.

    jmp     .next_byte                          ; Continue to the next character.

.end:
    ;pop     eax                                 ; Restore EAX.
    ;pop     edx                                 ; Restore EDI.

    ;pop     ebp                                 ; And restore EBP.
    ret                                         ; Done.


; Dumps memory to the COM2 serial port.
;
; Parameters:
;   ESI - address of memory.
;   ECX - number of bytes.
Com2Dump:
    ;push    ebp                                 ; Preserve EBP... For whatever reason.

    ;push    edx                                 ; Preserve EDX.
    ;push    ebx                                 ; Preserve EBX.
    ;push    eax                                 ; Preserve EAX.

    mov     ebx, ecx                            ; EBX will now contain the number of bytes.

.next_byte:
    cmp     ebx, 0                              ; If the are no more bytes to dump...
    jle     .end                                ; End.

.check:
    ; CHECK FIFO
    mov     dx, 0x2F8 + 5                       ; This is where I check if the buffer's empty.
    in      al, dx                              ; Grab the data...

    bt      ax, 5                               ; If the bit's clear (buffer non-empty)
    jnc     .check                              ; Try again!

    ;   Dump no more than 14 bytes!
    mov     ecx, 14                             ; Max 14 bytes.
    cmp     ecx, ebx                            ; Compare ECX with EBX.
    jle     .write                              ; If ECX < EBX then skip the next statement
    mov     ecx, ebx                            ; ECX will not me larger than EBX this way.

.write:
    sub     ebx, ecx                            ; Subtract the number of bytes to be written from the number of bytes left.

    mov         dx, 0x2F8                       ; Set the correct port for output.
    rep outsb                                   ; Output ECX bytes to COM2.

    jmp     .next_byte                          ; Continue to the next character.

.end:
    ;pop     eax                                 ; Restore EAX.
    ;pop     ebx                                 ; Restore EBX.
    ;pop     edx                                 ; Restore EDI.

    ;pop     ebp                                 ; And restore EBP.
    ;mov     rax, PostDump
    jmp     rax
    ret                                         ; Done.

