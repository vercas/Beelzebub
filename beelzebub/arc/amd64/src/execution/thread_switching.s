section .text
bits 64

global SwitchThread

SwitchThread:
    push rbp
    push r15
    push r14
    push r13
    push r12
    push rdx
    push rcx
    push rbx
    push rax
    pushf

    mov [rdi], rsp      ;   This is supposed to save the current stack
                        ;   pointer to whatever is pointed by the first
                        ;   argument.
.restore:
    mov rsp, [rsi]      ;   This is supposed to load the new stack pointer.

    popf
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp

    ret
