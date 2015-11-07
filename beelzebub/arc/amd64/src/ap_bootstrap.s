;   This file attempts to make a switch from real mode to long mode directly.
;
;

extern kmain_ap

extern KernelGdtPointer

extern ApStackTopPointer
extern ApInitializationLock

global ApBootstrapBegin
global ApBootstrapEnd
global BootstrapPml4Address

;   The physical location of this code.
%define PHYSADDR 0x1000

;   This will be used by the linker, mainly.
section .ap_bootstrap

bits 16     ;   Start small.

align 16    ;   Make sure there is some alignment going.

;   Marks the beginning of the AP bootstrap code.
ApBootstrapBegin:
    cli
    ;   Make sure the core doesn't get interrupted while doing its business.

    jmp 0x0:.realMode - ApBootstrapBegin + PHYSADDR
    ;   Clear code segment by jumping to absolute address in segment 0.

.realMode:
    mov     eax, 0x80000001
    cpuid
    mov     edi, edx
    ;   The NX bit support (bit 20) and PGE (bit 26) are needed.

    mov     eax, cr4
    bts     eax, 5
    ;   Enable PAE

    bt      edi, 26
    jnc     .write_cr4 - ApBootstrapBegin + PHYSADDR
    bts     eax, 7
    ;   And PGE (global pages) if available.

.write_cr4:
    mov     cr4, eax
    ;   And flush.

    mov     eax, [BootstrapPml4Address - ApBootstrapBegin + PHYSADDR]
    mov     cr3, eax
    ;   Load the PML4.

    mov     ecx, 0xC0000080
    rdmsr
    bts     eax, 8
    ;   Enable long mode

    bt      edi, 20
    jnc     .write_efer - ApBootstrapBegin + PHYSADDR
    bts     eax, 11
    ;   And NX/XD bit if available.

.write_efer:
    wrmsr
    ;   Writes the value to IA32_EFER

    mov     eax, cr0
    bts     eax, 0
    bts     eax, 31
    mov     cr0, eax
    ;   Protected mode and paging.

    lgdt    [cs:ApBootstrapGdt64R - ApBootstrapBegin + PHYSADDR]
    ;   Load a GDT which contains long mode segments.

    jmp     0x08:.long - ApBootstrapBegin + PHYSADDR
    ;   Long jump into protected mode!

bits 64 ;   Then long mode directly.

.long:
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    ;   Make sure the data segments are correct.
    ;   The following code involves data fetching, so better safe than sorry?

    mov     rax, KernelGdtPointer
    lgdt    [rax]
    ;   Load the GDT for domain 0.

    mov     rax, ApStackTopPointer
    mov     rsp, qword [rax]
    ;   The entry point will surely need a stack.

    mov     rax, ApInitializationLock
    mov     dword [rax], 0
    ;   Tell the BSP that initialization is complete.

    mov     rax, kmain_ap
    jmp     rax
    ;   Finally, jump to kernel.

.halt_indefinitely:
    hlt
    jmp     .halt_indefinitely
    ;   Eh, just makin' sure?

;   And now, data.

align 16    ;   Make it all tidy.

ApBootstrapGdt64R:
    .Limit: dw 0x17
    .Base: dd ApBootstrapGdt64 - ApBootstrapBegin + PHYSADDR

align 8

ApBootstrapGdt64:
    .null:
        dq 0x0000000000000000
    .kernel_code:       ;   64-bit kernel code
        dq 0x0020980000000000
    .kernel_data:       ;   64-bit kernel data
        dq 0x0020920000000000
;   This is a trimmed down version of the GDT probided by Jegudiel.

align 16

BootstrapPml4Address:
    dq 0

align 16    ;   Just make sure things are neat and aligned.

;   Marks the end of the AP bootstrap code.
ApBootstrapEnd:

db 0
;   Dummy.
