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

;   It seems that not backing up anything on the user stack was a good idea,
;   considering the fact that userland could change the stack pointer to
;   anything prior to a syscall.

section .text
bits 64

global SyscallEntry_64
extern SyscallCommon

;   It is absolutely vital that the time & instructions between syscall/sysret
;   and stack swaps is minimal, because NMIs can still occur.

;   Userland should know R12 is clobbered.
SyscallEntry_64:
    swapgs
    ;   Grab kernel GS base ASAP.

    mov     qword [gs:CpuData.SyscallUserlandStack], rsp
    mov     rsp, qword [gs:CpuData.SyscallStack]
    ;   Back up user stack pointer into core data and retrieve the kernel
    ;   stack pointer, also ASAP.

    push    rcx
    push    r11
    ;   Back up return RIP and RFLAGS on kernel stack.

    push    rax
    ;   This is the syscall selection.

    mov     rax, qword [gs:CpuData.SyscallUserlandStack]
    push    rax
    ;   This is the userland stack.

    sti
    ;   Syscalls are interruptible.

    xchg    r10, rcx
    ;   RCX contained return RIP and R10 contained the fourth argument.
    ;   They're needed the other way around.

    call    SyscallCommon
    ;   Just a vanilla call. The arguments are already in the right registers
    ;   and stack positions.

    pop     rdi
    ;   This retrieves the userland stack.

    add     rsp, 8
    ;   "Pop" the syscall selection.

    ;   No need to swap R10 and RCX again.

    pop     r11
    pop     rcx
    ;   Restore return RFLAGS and RIP from kernel stack.

    xor     esi, esi
    xor     edx, edx
    xor     r8d, r8d
    xor     r9d, r9d
    ;   Make sure that information is not leaked!

    cli
    ;   Will disable interrupts until sysret.

    swapgs
    ;   Restore user GS base.

    mov     rsp, rdi
    ;   Restore user stack.

    o64 sysret
