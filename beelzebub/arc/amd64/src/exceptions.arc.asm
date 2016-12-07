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

%include "beel/structs.kernel.inc"

section .text
bits 64

extern GetExceptionContext

global EnterExceptionContext
global SwapToExceptionContext

;   Performs the transition into an exception context.
;   Arguments:
;       RDI: Address of context to enter;
;   When re-entered through the swap or resume pointers, arguments should remain
;   valid!
EnterExceptionContext:
    push    rbp
    mov     rbp, rsp
    ;   Base pointer... For correctness.

    mov     qword [rdi + ExceptionContext.Status], 3
    ;   Marks the context as not ready.

    mov     eax, 0
    mov     [rdi + ExceptionContext.Payload], rax
    ;   Sets the payload to null.

    mov     rax, [rsp]
    mov     [rdi + ExceptionContext.RBP], rax
    ;   Save RBP.

    mov     [rdi + ExceptionContext.R15], r15
    mov     [rdi + ExceptionContext.R14], r14
    mov     [rdi + ExceptionContext.R13], r13
    mov     [rdi + ExceptionContext.R12], r12
    mov     [rdi + ExceptionContext.RCX], rcx
    mov     [rdi + ExceptionContext.RBX], rbx
    ;   These are callee-saved registers.

    mov     [rdi + ExceptionContext.RSP], rsp
    mov     rdx, .resume
    mov     [rdi + ExceptionContext.ResumePointer], rdx
    mov     rax, .swap
    mov     [rdi + ExceptionContext.SwapPointer], rax
    ;   Stores the current stack pointer, the resume pointer and the swap pointer.

    mov     rdx, [rsp + 8]
    mov     [rdi + ExceptionContext.ReturnAddress], rdx
    ;   This preserves the return address.

    call GetExceptionContext
    ;   Now RAX will hold the pointer to the current exception context's address.

    mov     rdx, [rax]
    mov     [rdi + ExceptionContext.Previous], rdx
    ;   This sets the entering context's previous pointer to the current context

    mov     qword [rdi + ExceptionContext.Status], 0
    ;   Marks the context as active.
    
    mov     [rax], rdi
    ;   And this sets the current exception context pointer to the address of
    ;   the entering context.

    mov     eax, 1
    ;   This is the return value for the standard call of this function.

    pop     rbp
    ret

.swap:
    mov     qword [rdi + ExceptionContext.Status], 2
    ;   Marks the context as handling.

    mov     rsp, [rdi + ExceptionContext.RSP]
    mov     r15, [rdi + ExceptionContext.R15]
    mov     r14, [rdi + ExceptionContext.R14]
    mov     r13, [rdi + ExceptionContext.R13]
    mov     r12, [rdi + ExceptionContext.R12]
    mov     rbp, [rdi + ExceptionContext.RBP]
    mov     rcx, [rdi + ExceptionContext.RCX]
    mov     rbx, [rdi + ExceptionContext.RBX]
    ;   This restores stack pointer and all calee-saved registers.

    ;   Swapping ends with resuming.
    ;   When resumed by an exception handler, normally it's going to be the one
    ;   restoring registers and marking it as handling.

.resume:
    mov     [rsp], rbp
    ;   This is just for consistency...

    mov     rax, [rdi + ExceptionContext.ReturnAddress]
    mov     [rsp + 8], rax
    ;   This restores the return address.

    ;   The previous exception context is not restored as the current context
    ;   yet. This is so new exceptions can be thrown and link this context's
    ;   exception as the previous/cause.

    mov     eax, 0
    ;   This is the return value for resuming...

    pop     rbp
    ret

;   Swaps to the catch block of an exception context.
;   Arguments:
;       RDI: Address of context;
SwapToExceptionContext:
    jmp     [rdi + ExceptionContext.SwapPointer]
