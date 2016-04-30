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

section .text
bits 64

global GoToRing3_64

;   Performs the transition into ring 3.
;   Arguments:
;       RDI: Entry point;
;       RSI: Stack top;
GoToRing3_64:
    mov     ebx, 0x33
    mov     ds, bx
    mov     es, bx
    ;   Changes the data segments to 64-bit user code.

    xor     r8d, r8d
    xor     r9d, r9d
    xor     r10, r10
    xor     r11, r11
    xor     r12, r12
    xor     r13, r13
    xor     r14, r14
    xor     r15, r15
    ;   Get rid of information in those registers.

    sub     rsi, 8
    mov     qword [rsi], r8
    ;   Set up proper stack alignment, and push a null return address.

    push    rbx
    push    rsi
    push    qword 0x202
    push    qword 0x2B
    push    rdi
    ;   Set up the stack for IRETQ.

    xor     eax, eax
    xor     ebx, ebx
    xor     ecx, ecx
    xor     edx, edx
    xor     ebp, ebp
;   And finish the rest of the registers.

    ;   RDI, RSI, RSP and RIP don't matter. They don't leak any info.

    cli

    swapgs

    iretq
