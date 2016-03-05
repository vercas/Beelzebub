; Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

global BigIntAdd
global BigIntAdd2

align 16
;   Arguments:
;       RDI: uint32_t       * dst
;       RSI: uint32_t const * src
;       RDX: uint32_t         size
;       ECX: bool/uint32_t    cin
BigIntAdd2:
    push    rbp
    mov     rbp, rsp
    push    rbx

    mov     eax, ecx
    xor     ecx, ecx
    ;   EAX becomes the carry, RCX is used as offset.

    cmp     rdx, 2
    jl      .lt_2
    ;   If the size is less than two, skip.

align 16
.two_at_once:

    mov     rbx, qword [rsi + rcx * 4]  ;   Get source.
    bt      eax, 0                      ;   Get carry in.
    adc     qword [rdi + rcx * 4], rbx  ;   *dst += *src
    setc    al                          ;   Store carry out.

    add     rcx, byte 2                 ;   Increment offset by 2.

    mov     rbx, rdx                    ;   Grab size...
    sub     rbx, rcx                    ;   Minus offset...
    cmp     rbx, 1                      ;   If more than one dword is left
    jg      .two_at_once                ;   Repeat.

.lt_2:

    mov     rbx, rdx                    ;   Grab size...
    sub     rbx, rcx                    ;   Minus current offset... (may have been modified by previous loop)
    cmp     rbx, 1                      ;   If less than one dword is left
    jl      .lt_1                       ;   Skip this last step.

    mov     ebx, dword [rsi + rcx * 4]  ;   Get source.
    bt      eax, 0                      ;   Get carry in.
    adc     dword [rdi + rcx * 4], ebx  ;   *dst += *src
    setc    al                          ;   Store carry out.

.lt_1:

    pop     rbx
    pop     rbp
    ret

align 16
;   Arguments:
;       RDI: uint32_t       * dst
;       RSI: uint32_t const * src1
;       RDX: uint32_t const * src2
;       ECX: uint32_t         size
;       R8D: bool/uint32_t    cin
BigIntAdd:
    cmp     rdi, rsi
    jne     .skip_shortcut1
    ;   If the destination is the same as the first source...

    mov     rsi, rdx
    mov     edx, ecx
    mov     ecx, r8d
    ;   Shift parameters left, overriding the first source.

    jmp     BigIntAdd2  ;   return BigIntAdd2(dst, src2, size, cin);

align 16
.skip_shortcut1:
    cmp     rdi, rdx
    jne     .skip_shortcut2
    ;   If the destination is the same as the second source...

    mov     edx, ecx
    mov     ecx, r8d
    ;   Shift parameters left, overriding the second source.

    jmp     BigIntAdd2  ;   return BigIntAdd2(dst, src1, size, cin);

align 16
.skip_shortcut2:

    push    rbp
    mov     rbp, rsp
    push    rbx

    xor     ebx, ebx
    mov     eax, r8d
    ;   EAX becomes the carry, RBX is used as an offset.

    cmp     rcx, 2
    jl      .lt_2
    ;   If the size is less than two, skip.

align 16
.two_at_once:

    mov     r8, qword [rsi + rbx * 4]   ;   Get source 1.
    mov     r9, qword [rdx + rbx * 4]   ;   Get source 2.
    bt      eax, 0                      ;   Get carry in.
    adc     r8, r9                      ;   temp = *src1 + *src2
    setc    al                          ;   Store carry out.
    mov     qword [rdi + rbx * 4], r8   ;   *dst = temp

    add     rbx, byte 2                 ;   Increment offset by 2.

    mov     r9, rcx                     ;   Grab size...
    sub     r9, rbx                     ;   Minus offset...
    cmp     r9, 1                       ;   If more than one dword is left
    jg      .two_at_once                ;   Repeat.

.lt_2:

    mov     r8, rcx                     ;   Grab size...
    sub     r8, rbx                     ;   Minus current offset... (may have been modified by previous loop)
    cmp     r8, 1                       ;   If less than one dword is left
    jl      .lt_1                       ;   Skip this last step.

    mov     r8d, dword [rsi + rbx * 4]  ;   Get source 1.
    mov     r9d, dword [rdx + rbx * 4]  ;   Get source 2.
    bt      eax, 0                      ;   Get carry in.
    adc     r8d, r9d                    ;   temp = *src1 + *src2
    setc    al                          ;   Store carry out.
    mov     dword [rdi + rbx * 4], r8d  ;   *dst = temp

.lt_1:

    pop     rbx
    pop     rbp
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

global BigIntSub
global BigIntSub2

align 16
;   Arguments:
;       RDI: uint32_t       * dst
;       RSI: uint32_t const * src
;       RDX: uint32_t         size
;       ECX: bool/uint32_t    cin
BigIntSub2:
    push    rbp
    mov     rbp, rsp
    push    rbx

    mov     eax, ecx
    xor     ecx, ecx
    ;   EAX becomes the carry, RCX is used as offset.

    cmp     rdx, 2
    jl      .lt_2
    ;   If the size is less than two, skip.

align 16
.two_at_once:

    mov     rbx, qword [rsi + rcx * 4]  ;   Get source.
    bt      eax, 0                      ;   Get carry in.
    sbb     qword [rdi + rcx * 4], rbx  ;   *dst += *src
    setc    al                          ;   Store carry out.

    add     rcx, byte 2                 ;   Increment offset by 2.

    mov     rbx, rdx                    ;   Grab size...
    sub     rbx, rcx                    ;   Minus offset...
    cmp     rbx, 1                      ;   If more than one dword is left
    jg      .two_at_once                ;   Repeat.

.lt_2:

    mov     rbx, rdx                    ;   Grab size...
    sub     rbx, rcx                    ;   Minus current offset... (may have been modified by previous loop)
    cmp     rbx, 1                      ;   If less than one dword is left
    jl      .lt_1                       ;   Skip this last step.

    mov     ebx, dword [rsi + rcx * 4]  ;   Get source.
    bt      eax, 0                      ;   Get carry in.
    sbb     dword [rdi + rcx * 4], ebx  ;   *dst += *src
    setc    al                          ;   Store carry out.

.lt_1:

    pop     rbx
    pop     rbp
    ret

align 16
;   Arguments:
;       RDI: uint32_t       * dst
;       RSI: uint32_t const * src1
;       RDX: uint32_t const * src2
;       ECX: uint32_t         size
;       R8D: bool/uint32_t    cin
BigIntSub:
    cmp     rdi, rsi
    jne     .skip_shortcut1
    ;   If the destination is the same as the first source...

    mov     rsi, rdx
    mov     edx, ecx
    mov     ecx, r8d
    ;   Shift parameters left, overriding the first source.

    jmp     BigIntSub2  ;   return BigIntAdd2(dst, src2, size, cin);

align 16
.skip_shortcut1:
    cmp     rdi, rdx
    jne     .skip_shortcut2
    ;   If the destination is the same as the second source...

    mov     edx, ecx
    mov     ecx, r8d
    ;   Shift parameters left, overriding the second source.

    jmp     BigIntSub2  ;   return BigIntAdd2(dst, src1, size, cin);

align 16
.skip_shortcut2:

    push    rbp
    mov     rbp, rsp
    push    rbx

    xor     ebx, ebx
    mov     eax, r8d
    ;   EAX becomes the carry, RBX is used as an offset.

    cmp     rcx, 2
    jl      .lt_2
    ;   If the size is less than two, skip.

align 16
.two_at_once:

    mov     r8, qword [rsi + rbx * 4]   ;   Get source 1.
    mov     r9, qword [rdx + rbx * 4]   ;   Get source 2.
    bt      eax, 0                      ;   Get carry in.
    sbb     r8, r9                      ;   temp = *src1 + *src2
    setc    al                          ;   Store carry out.
    mov     qword [rdi + rbx * 4], r8   ;   *dst = temp

    add     rbx, byte 2                 ;   Increment offset by 2.

    mov     r9, rcx                     ;   Grab size...
    sub     r9, rbx                     ;   Minus offset...
    cmp     r9, 1                       ;   If more than one dword is left
    jg      .two_at_once                ;   Repeat.

.lt_2:

    mov     r8, rcx                     ;   Grab size...
    sub     r8, rbx                     ;   Minus current offset... (may have been modified by previous loop)
    cmp     r8, 1                       ;   If less than one dword is left
    jl      .lt_1                       ;   Skip this last step.

    mov     r8d, dword [rsi + rbx * 4]  ;   Get source 1.
    mov     r9d, dword [rdx + rbx * 4]  ;   Get source 2.
    bt      eax, 0                      ;   Get carry in.
    sbb     r8d, r9d                    ;   temp = *src1 + *src2
    setc    al                          ;   Store carry out.
    mov     dword [rdi + rbx * 4], r8d  ;   *dst = temp

.lt_1:

    pop     rbx
    pop     rbp
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
