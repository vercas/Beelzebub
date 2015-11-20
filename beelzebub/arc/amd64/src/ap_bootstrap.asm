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

extern kmain_ap

extern KernelGdtPointer

extern ApStackTopPointer

extern ApInitializationLock1
extern ApInitializationLock2
extern ApInitializationLock3

global ApBootstrapBegin
global ApBootstrapEnd
global BootstrapPml4Address
global ApBreakpointCookie

;   The physical location of this code.
%define PHYSADDR 0x1000

%macro debugchar 1
    mov     dx, 0x3F8
    mov     al, %1
    out     dx, al
%endmacro

%macro breakpoint 1
    mov     word [ApBreakpointCookie - ApBootstrapBegin + PHYSADDR], 1
%%check:
    pause
    cmp     word [ApBreakpointCookie - ApBootstrapBegin + PHYSADDR], 0
    jne     %%check
%endmacro

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

    ;debugchar 'A'
    ;breakpoint dummy

    mov     eax, cr4
    bts     eax, 5
    ;   Enable PAE.

    bt      edi, 26
    jnc     .write_cr4
    bts     eax, 7
    ;   And PGE (global pages) if available.

.write_cr4:
    mov     cr4, eax
    ;   And flush.

    ;debugchar 'B'

    mov     eax, [BootstrapPml4Address - ApBootstrapBegin + PHYSADDR]
    mov     cr3, eax
    ;   Load the PML4.

    ;debugchar 'C'

    mov     ecx, 0xC0000080
    rdmsr
    bts     eax, 8
    ;   Enable long mode

    bt      edi, 20
    jnc     .write_efer
    bts     eax, 11
    ;   And NX/XD bit if available.

.write_efer:
    wrmsr
    ;   Writes the value to IA32_EFER

    ;debugchar 'D'

    mov     eax, cr0
    bts     eax, 0
    bts     eax, 31
    mov     cr0, eax
    ;   Protected mode and paging.

    ;debugchar 'E'

    lgdt    [cs:ApBootstrapGdt64R - ApBootstrapBegin + PHYSADDR]
    ;   Load a GDT which contains long mode segments.

    ;debugchar 'F'

    jmp     0x08:.long - ApBootstrapBegin + PHYSADDR
    ;   Long jump into protected mode!

bits 64 ;   Then long mode directly.

.long:
    ;debugchar 'G'

    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    ;   Make sure the data segments are correct.
    ;   The following code involves data fetching, so better safe than sorry?

    ;debugchar 'H'

    mov     rax, KernelGdtPointer
    lgdt    [rax]
    ;   Load the GDT for domain 0.

    ;debugchar 'I'

    mov     rax, ApStackTopPointer
    mov     rsp, qword [rax]
    ;   The entry point will surely need a stack.

    ;debugchar 'J'

    mov     rax, ApInitializationLock1
    mov     rbx, ApInitializationLock2
    mov     rcx, ApInitializationLock3
    ;   Addresses of the locks forming the initialization barrier.

    mov     dword [rax], 0
    ;   Tell the BSP that the AP has started up properly.

.check_entry_barrier:
    pause
    cmp     dword [rbx], 0
    jne     .check_entry_barrier
    ;   Wait for the BSP to lower the entry barrier.

    mov     dword [rcx], 0
    ;   Tell the BSP that the AP got past the barrier.

    ;debugchar 'K'

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

ApBreakpointCookie:
    dq 0

;   Marks the end of the AP bootstrap code.
ApBootstrapEnd:

db 0
;   Dummy.
