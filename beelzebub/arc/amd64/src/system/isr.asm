; Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.
;
;
; Developed by: Alexandru-Mihai Maftei
; aka Vercas
; http://vercas.com | https://github.com/vercas/Beelzebub
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to
; deal with the Software without restriction, including without limitation the
; rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
; sell copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
;   * Redistributions of source code must retain the above copyright notice,
;     this list of conditions and the following disclaimers.
;   * Redistributions in binary form must reproduce the above copyright
;     notice, this list of conditions and the following disclaimers in the
;     documentation and/or other materials provided with the distribution.
;   * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
;     its contributors may be used to endorse or promote products derived from
;     this Software without specific prior written permission.
;
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
; WITH THE SOFTWARE.
;
; ---
;
; You may also find the text of this license in "LICENSE.md", along with a more
; thorough explanation regarding other files.

global InterruptHandlers
global InterruptEnders

%assign i 0
%rep 256
    global IsrStub %+ i
    %assign i i+1
%endrep

global IsrStubsBegin
global IsrStubsEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .text
bits 64

DEFAULT REL

align 16

IsrCommonStub:
    ;   Upon entry, the lower byte of RCX will contain the interrupt vector.
    ;   That value is zero-extend into the whole register.
    movzx   rcx, cl

    push    rax
    push    rbx
    push    rdx
    push    rbp
    push    rdi
    push    rsi
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
    ;   Store general-purpose registers, except RCX.

    xor     rax, rax
    mov     ax, ds
    push    rax
    ;   Save data segment.

    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    ;   Make sure the data segments are the kernel's, before accessing data below.

    mov     rbx, InterruptHandlers
    mov     rdx, qword [rbx + rcx * 8]
    ;   Handler pointer.

    test    rdx, rdx
    jz      .skip
    ;   A null handler means the call is skipped.

    mov     rbx, InterruptEnders
    mov     rsi, qword [rbx + rcx * 8]
    ;   Ender pointer.

    mov     rdi, rsp
    ;   Stack pointer as first parameter (IsrState *)

    mov     rax, [rsp + 0x90]
    cmp     al, byte 0x8 
    je      .skip_swap_1
    ;   If the old code segment was the kernel's, skip the swapping.

    ;   The continuous path is the one that includes swapping because most CPU
    ;   time should be spent in the userland anyway.

    swapgs

.skip_swap_1:
    mov     ebp, 0
    ;   The base pointer has to be 0, so the interrupt handler's stack frames do
    ;   not link to the userland frames.
    
    ;   At this point, the arguments given are the following:
    ;   1. RDI = State pointer
    ;   2. RSI = Ender pointer
    ;   3. RDX = Handler pointer
    ;   4. RCX = Vector
    call    rdx
    ;   Call handler. Preserves RBP by convention.

    mov     rax, [rsp + 0x90]
    cmp     al, byte 0x8 
    je      .skip_swap_2
    ;   The code segment may have changed. If it's the kernel's, no swap.

    swapgs

.skip_swap_2:
.skip:
    pop     rax 
    mov     es, ax
    mov     ds, ax
    ;   Simply restore the data segments.

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rsi
    pop     rdi
    pop     rbp
    pop     rdx
    pop     rbx
    pop     rax
    ;   Restore general-purpose registers, except RCX.

    pop     rcx
    ;   Pop RCX

    add     rsp, 8
    ;   "Pop" error code.

    iretq

%macro ISR_NOERRCODE 1
    IsrStub%1:
        push    qword 0
        push    rcx
        mov     cl, %1
        jmp     IsrCommonStub
    align 16
%endmacro

%macro ISR_ERRCODE 1
    IsrStub%1:
        push    rcx
        mov     cl, %1
        jmp     IsrCommonStub
    align 16
%endmacro

align 16

IsrStubsBegin:

    ISR_NOERRCODE 0
    ISR_NOERRCODE 1
    ISR_NOERRCODE 2
    ISR_NOERRCODE 3
    ISR_NOERRCODE 4
    ISR_NOERRCODE 5
    ISR_NOERRCODE 6
    ISR_NOERRCODE 7
    ISR_ERRCODE   8
    ISR_NOERRCODE 9
    ISR_ERRCODE   10
    ISR_ERRCODE   11
    ISR_ERRCODE   12
    ISR_ERRCODE   13
    ISR_ERRCODE   14
    ISR_NOERRCODE 15
    ISR_NOERRCODE 16
    ISR_NOERRCODE 17
    ISR_NOERRCODE 18
    ISR_NOERRCODE 19
    ISR_NOERRCODE 20
    ISR_NOERRCODE 21
    ISR_NOERRCODE 22
    ISR_NOERRCODE 23
    ISR_NOERRCODE 24
    ISR_NOERRCODE 25
    ISR_NOERRCODE 26
    ISR_NOERRCODE 27
    ISR_NOERRCODE 28
    ISR_NOERRCODE 29
    ISR_NOERRCODE 30
    ISR_NOERRCODE 31

    %assign i 32
    %rep 224
        ISR_NOERRCODE i
        %assign i i+1
    %endrep

IsrStubsEnd:
    nop ;   Dummy.

DEFAULT ABS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .bss

align 16

InterruptHandlers:
    resb 8 * 256
InterruptEnders:
    resb 8 * 256
