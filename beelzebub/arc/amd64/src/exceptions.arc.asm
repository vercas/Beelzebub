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

extern GetExceptionContext

global EnterExceptionContext

;   Performs the transition into an exception context.
;   Arguments:
;       RDI: Address of context to enter;
;       RSI: Address of pointer to the current context;
;   When re-entered through the swap or resume pointers, those two should remain
;   valid!
EnterExceptionContext:
    push    rbp
    mov     rbp, rsp
    ;   Base pointer... For correctness.

    mov     byte [rdi + 104], 0
    ;   Marks the context as not ready.

    mov     eax, 0
    mov     [rdi + 88], rax
    ;   Sets the payload to null.

    mov     [rdi + 48], r15
    mov     [rdi + 40], r14
    mov     [rdi + 32], r13
    mov     [rdi + 24], r12
    mov     [rdi + 16], rbp
    mov     [rdi +  8], rcx
    mov     [rdi     ], rbx
    ;   These are callee-saved registers.

    mov     [rdi + 56], rsp
    mov     rdx, .resume
    mov     [rdi + 64], rdx
    mov     rax, .swap
    mov     [rdi + 72], rax
    ;   Stores the current stack pointer, the resume pointer and the swap pointer.

    mov     rdx, [rsp]
    mov     [rdi + 80], rdx
    ;   This preserves the return address.

    call GetExceptionContext
    ;   Now RAX will hold the pointer to the current exception context's address.

    mov     rdx       , [rax]
    mov     [rdi + 96], rdx
    ;   This sets the entering context's previous pointer to the current context

    mov     byte [rdi + 104], 1
    ;   Marks the context as ready.
    
    mov     [rax     ], rdi
    ;   And this sets the current exception context pointer to the address of
    ;   the entering context.

    mov     eax, 1
    ;   This is the return value for the standard call of this function.

    pop     rbp
    ret

.swap:
    mov     rsp, [rdi + 56]
    mov     r15, [rdi + 48]
    mov     r14, [rdi + 40]
    mov     r13, [rdi + 32]
    mov     r12, [rdi + 24]
    mov     rbp, [rdi + 16]
    mov     rcx, [rdi +  8]
    mov     rbx, [rdi     ]
    ;   This restores stack pointer and all calee-saved registers.

    ;   Swapping ends with resuming.
    ;   When resumed by an exception handler, normally it's going to be the one
    ;   restoring registers and such.

.resume:

    ;   Now that the context is resuming, it must be deactivated so it doesn't
    ;   re-enter accidentally.

    mov     byte [rdi + 104], 0
    ;   Marks the context as not ready.

    mov     rax, [rdi + 80]
    mov     [rsp], rax
    ;   This restores the return address.

    ;   The previous exception context is not restored as the current context
    ;   yet. This is so new exceptions can be thrown and link this context's
    ;   exception as the previous/cause.

    mov     eax, 0
    ;   This is the return value for resuming...

    pop     rbp
    ret
