; Copyright (c) 2012 by Lukas Heidemann <lukasheidemann@gmail.com>
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
; NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

; Protected mode entry point for the BSP.
;
; Directly entered by the multiboot bootloader; a pointer to the multiboot info
; tables is stored in EBX.
boot32_bsp:
	cli									; Clear interrupts
	mov dword [multiboot_info], ebx		; Store multiboot pointer

	mov esp, boot32_stack_bsp			; Load BSP stack
	add esp, 0x1000						; Advance to top of stack

    call boot32_gdt                     ; Setup 64 bit GDT
	call boot32_map						; Setup mapping
	call boot32_common					; Common bootstrap

    mov ax, 0x10                        ; Setup data segment selectors
    mov ds, ax
    mov ss, ax
    mov gs, ax
    mov fs, ax
    mov es, ax

	jmp 0x8:main_bsp					; Far jump

; Protected mode entry point for the APs.
;
; Entered by the real mode bootstrap code.
boot32_ap:
	cli									; Clear interrupts

	add dword [heap_top], 0x1000		; Allocate stack from heap
	mov esp, dword [heap_top] 			; Load top of allocated stack

	call boot32_common					; Common bootstrap
	jmp 0x8:main_ap						; Far jump

; Common initialization for both, the BSP and the APs.
boot32_common:
	mov eax, 0x80000001					; Check for long mode support
	cpuid								; using CPUID
	and edx, (1 << 29)
	cmp edx, 0
	jne .long_mode_supported			; Long mode supported

	mov edi, boot32_msg_nolm			; Panic
	call boot32_panic

.long_mode_supported:
	mov eax, cr4						; Enable PAE
	or eax, 1 << 5
	mov cr4, eax

	in al, 0x92							; Enable A20
	or al, 2
	out 0x92, al

	mov ecx, 0xC0000080					; Enable LM in MSR
	rdmsr
	or eax, 1 << 8
	wrmsr

	mov eax, gdt_pointer				; Load the 64 bit GDT pointer
	lgdt [eax]							; Load the 64 bit GDT

	mov eax, page_pml4					; Load PML4 address
	mov cr3, eax						; Load the PML4

	mov eax, cr0						; Enable paging
	or eax, 1 << 31
	mov cr0, eax
	ret

; Sets up 64 bit GDT.
boot32_gdt:
    mov edi, gdt_data                   ; Target pointer
    mov esi, boot32_gdt_data            ; Source pointer
    mov ecx, boot32_gdt_data.end        ; Length
    sub ecx, esi
.next:
    lodsb
    stosb
    dec ecx
    cmp ecx, 0
    jne .next
    ret

; Sets up identity mapping.
boot32_map:
	mov eax, page_idn_pdp                  ; Map identity PDP in PML4
	or eax, 0b11                           ; Present + Writable
	mov dword [page_pml4], eax             ; First entry in PML4

	mov edi, page_idn_pdp                  ; Map PDPEs to PDs
	mov ecx, 64                            ; Write 64 entries
	mov eax, page_idn_pd
	or eax, 0b11

.next_pdpe:
	stosd                                  ; Write lower DWORD of entry
	add edi, 4                             ; Skip upper DWORD
	add eax, 0x1000                        ; Advance by one page
	dec ecx                                ; Decrease number of remaining entries

	cmp ecx, 0                             ; Entries left?
	jne .next_pdpe

	mov eax, 0b10000011                    ; Present + Writable + 2MB pages; start at 0x0
	mov edi, page_idn_pd                   ; Identity map PDs to memory
	mov ecx, 512 * 64                      ; 512 entries in 64 PDs

.next_pde:
	stosd                                  ; Write lower DWORD of entry
	add edi, 4                             ; Skip upper DWORD
	add eax, 0x200000                      ; Advance by one 2MB page
	dec ecx	                               ; Decrease number of remaining entries

	cmp ecx, 0                             ; Entries left?
	jne .next_pde
	ret

; Writes a message to the screen, then enters an infinite loop.
;
; Parameters:
;	ESI pointer to the message to write.
boot32_panic:
	mov edi, 0xb8000                       ; Write to VGA video memory
.next_byte:
	lodsb                                  ; Load character
	cmp al, 0                              ; Null?
	je .end
	or ax, 0x00F0                          ; Black on white
	stosb                                  ; Write to screen
	jmp .next_byte                         ; Continue with next byte

.end:
	jmp $                                  ; Infinite loop

section .rodata
boot32_msg_nolm: db "64 bit long mode not supported on this CPU.", 0

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
