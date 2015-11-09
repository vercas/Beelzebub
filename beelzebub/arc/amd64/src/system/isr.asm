section .text
bits 64

DEFAULT REL

global IsrGates
global InterruptHandlers
global InterruptEnders

%assign i 0
%rep 256
    global isr_stub %+ i
    %assign i i+1
%endrep

align 16

isr_stub_common:
    push    rax                         ;   Store general purpose registers
    push    rbx
    push    rcx
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

    xor     rax, rax
    mov     ax, ds                      ;   Save data segment.
    push    rax

    mov     ax, 0x10                    ;   Load kernel's data segment
    mov     ds, ax
    mov     es, ax

    mov     rdi, rsp                    ;   Stack pointer as first parameter (ISR state)
    mov     rcx, qword [rdi + 128]      ;   Grab the interrupt vector...
    mov     rbx, InterruptHandlers
    mov     rdx, qword [rbx + rcx * 8]  ;   Handler...
    mov     rbx, InterruptEnders
    mov     rsi, qword [rbx + rcx * 8]  ;   And ender.

    test    rdx, rdx                    ;   A null handler means do nuthin'.
    jz      .skip                 ;   So it skips the call.

    ;   At this point, the arguments given are the following:
    ;   1. RDI = State pointer
    ;   2. RSI = Ender pointer
    ;   3. RDX = Handler pointer
    ;   4. RCX = Vector
    call    rdx                            ;   Call handler

.skip:
    pop     rax                         ;   Restore data segments
    mov     es, ax
    mov     ds, ax

    pop     r15                         ;   Restore registers
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
    pop     rcx
    pop     rbx
    pop     rax
    add     rsp, 16                     ;   Remove error code and vector number

    iretq

%macro ISR_NOERRCODE 1
    isr_stub%1:
        cli
        push    qword 0
        push    qword %1
        jmp     isr_stub_common
%endmacro

%macro ISR_ERRCODE 1
    isr_stub%1:
        cli
        push    qword %1
        jmp     isr_stub_common
%endmacro

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

DEFAULT ABS

section .data

align 16

IsrGates:
    %assign i 0
    %rep 256
        dq isr_stub %+ i
        %assign i i+1
    %endrep

section .bss

InterruptHandlers:
    resb 8 * 256
InterruptEnders:
    resb 8 * 256
