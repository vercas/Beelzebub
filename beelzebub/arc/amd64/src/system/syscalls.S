/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#include <beel/structs.kernel.h>

//  It seems that not backing up anything on the user stack was a good idea,
//  considering the fact that userland could change the stack pointer to
//  anything prior to a syscall.

.section .text

.global SyscallEntry_64
.type SyscallEntry_64, @function

.extern SyscallCommon
// .extern SyscallUserlandStack

//  It is absolutely vital that the time & instructions between syscall/sysret
//  and stack swaps is minimal, because NMIs can still occur.

//  Userland should know R12 is clobbered.
SyscallEntry_64:
    swapgs
    //  Grab kernel GS base ASAP.

    //movq    %rax, %gs:(GeneralRegisters@tpoff + FIELDR(BeGeneralRegisters64, RAX))

    movq    %rsp, %gs:SyscallUserlandStack@tpoff
    movq    %gs:SyscallStack@tpoff, %rsp
    //  Back up user stack pointer into core data and retrieve the kernel
    //  stack pointer, also ASAP.

    subq    $sizeof(BeGeneralRegisters64), %rsp
    //  Ensure data on the stack is not going to get pwnd by interrupts.

    movq    %rax, FIELDR(BeGeneralRegisters64, ErrorCode)(%rsp)
    movq    %rax, FIELDR(BeGeneralRegisters64, RAX)(%rsp)
    //  This is the syscall selection.

    movq    %gs:SyscallUserlandStack@tpoff, %rax
    movq    %rax, FIELDR(BeGeneralRegisters64, RSP)(%rsp)
    //  This is the userland stack pointer.

    sti
    //  Syscalls are interruptible. The rest of this code is safe to interrupt as well.

    movq    %r11, FIELDR(BeGeneralRegisters64, RFLAGS)(%rsp)
    movq    %rcx, FIELDR(BeGeneralRegisters64, RIP)(%rsp)
    //  Back up return RIP and RFLAGS on kernel stack.

    movq    %rdx, FIELDR(BeGeneralRegisters64, RDX)(%rsp)
    movq    %rbx, FIELDR(BeGeneralRegisters64, RBX)(%rsp)
    movq    %rcx, FIELDR(BeGeneralRegisters64, RCX)(%rsp)
    movq    %rbp, FIELDR(BeGeneralRegisters64, RBP)(%rsp)
    movq    %rdi, FIELDR(BeGeneralRegisters64, RDI)(%rsp)
    movq    %rsi, FIELDR(BeGeneralRegisters64, RSI)(%rsp)
    movq    %r8,  FIELDR(BeGeneralRegisters64, R8 )(%rsp)
    movq    %r9,  FIELDR(BeGeneralRegisters64, R9 )(%rsp)
    movq    %r10, FIELDR(BeGeneralRegisters64, R10)(%rsp)
    movq    %r11, FIELDR(BeGeneralRegisters64, R11)(%rsp)
    movq    %r12, FIELDR(BeGeneralRegisters64, R12)(%rsp)
    movq    %r13, FIELDR(BeGeneralRegisters64, R13)(%rsp)
    movq    %r14, FIELDR(BeGeneralRegisters64, R14)(%rsp)
    movq    %r15, FIELDR(BeGeneralRegisters64, R15)(%rsp)
    //  Yes, these include two previous registers too.

    movq    $0x18, %rax
    movq    %rax, FIELDR(BeGeneralRegisters64, CS)(%rsp)
    //  Userland code segment.

    addq    $8, %rax
    movq    %rax, FIELDR(BeGeneralRegisters64, SS)(%rsp)
    movq    %rax, FIELDR(BeGeneralRegisters64, DS)(%rsp)
    //  Stack segment of userland.

    xchgq   %r10, %rcx
    //  RCX contained return RIP and R10 contained the fourth argument.
    //  They're needed the other way around.

    call    SyscallCommon
    //  Just a vanilla call. The arguments are already in the right registers
    //  and stack positions.

    movq    FIELDR(BeGeneralRegisters64, RFLAGS)(%rsp), %r11
    movq    FIELDR(BeGeneralRegisters64, RIP   )(%rsp), %rcx
    //  Back up return RIP and RFLAGS on kernel stack.

    movq    FIELDR(BeGeneralRegisters64, RBX)(%rsp), %rbx
    movq    FIELDR(BeGeneralRegisters64, RDX)(%rsp), %rdx
    movq    FIELDR(BeGeneralRegisters64, RBP)(%rsp), %rbp
    movq    FIELDR(BeGeneralRegisters64, RDI)(%rsp), %rdi
    movq    FIELDR(BeGeneralRegisters64, RSI)(%rsp), %rsi
    movq    FIELDR(BeGeneralRegisters64, R8 )(%rsp), %r8
    movq    FIELDR(BeGeneralRegisters64, R9 )(%rsp), %r9
    movq    FIELDR(BeGeneralRegisters64, R10)(%rsp), %r10
    movq    FIELDR(BeGeneralRegisters64, R12)(%rsp), %r12
    movq    FIELDR(BeGeneralRegisters64, R13)(%rsp), %r13
    movq    FIELDR(BeGeneralRegisters64, R14)(%rsp), %r14
    movq    FIELDR(BeGeneralRegisters64, R15)(%rsp), %r15
    //  Now they are excluded.

    cli
    //  Will disable interrupts until sysret.

    movq    FIELDR(BeGeneralRegisters64, RSP)(%rsp), %rsp
    //  Restore userland stack.

    swapgs
    //  And GS base.

    sysretq
