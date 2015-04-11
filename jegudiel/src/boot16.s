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

section .text16
bits 16

extern boot32_ap
global boot16_begin
global boot16_end

; The location this code will be moved to
%define TARGET 0x1000

; Marks the beginning of the 16 bit boot code
boot16_begin:

; Entry point for the APs starting in real mode.
boot16:
    jmp 0x0:.cs_cleared - boot16_begin + TARGET         ; Clear CS

.cs_cleared:
    cli                                                 ; Clear interrupts
    in al, 0x92                                         ; Activate A20
    or al, 2
    out 0x92, al
    lgdt [cs:boot16_gdtr - boot16_begin + TARGET]       ; Load GDT
    mov eax, cr0                                        ; Enable PM
    or al, 1
    mov cr0, eax
    jmp 0x8:boot16_trampoline - boot16_begin + TARGET   ; Jump to 32 bit trampoline

boot16_gdtr:
    .Limit: dw 0x17
    .Base: dd boot16_gdt - boot16_begin + TARGET

boot16_gdt:
    .Null: dw 0x0000, 0x0000, 0x0000, 0x0000
    .Code: dw 0xFFFF, 0x0000, 0x9A00, 0x00CF
    .Data: dw 0xFFFF, 0x0000, 0x9200, 0x00CF

bits 32
boot16_trampoline:
    mov eax, 0x10                   ; Setup data segment selectors
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov eax, boot32_ap                ; Jump to 32 bit bootstrap
    jmp eax

boot16_end:
