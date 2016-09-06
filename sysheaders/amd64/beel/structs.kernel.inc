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

struc SyscallRegisters64
.R15:       RESQ 1
.R14:       RESQ 1
.R13:       RESQ 1
.R12:       RESQ 1
.RFLAGS:    RESQ 1
.R10:       RESQ 1
.R9:        RESQ 1
.R8:        RESQ 1
.RSI:       RESQ 1
.RDI:       RESQ 1
.RSP:       RESQ 1
.RBP:       RESQ 1
.RDX:       RESQ 1
.RIP:       RESQ 1
.RBX:       RESQ 1
.RAX:       RESQ 1
.size:
endstruc

struc SyscallRegisters32
.ESI:   RESD 1
.EDI:   RESD 1
.ESP:   RESD 1
.EBP:   RESD 1
.EDX:   RESD 1
.ECX:   RESD 1
.EBX:   RESD 1
.EAX:   RESD 1
.size:
endstruc

struc Tss
.Reserved1:         RESD 1
.Rsp0:              RESQ 1
.Rsp1:              RESQ 1
.Rsp2:              RESQ 1
.Reserved2:         RESD 2
.Ist1:              RESQ 1
.Ist2:              RESQ 1
.Ist3:              RESQ 1
.Ist4:              RESQ 1
.Ist5:              RESQ 1
.Ist6:              RESQ 1
.Ist7:              RESQ 1
.Reserved3:         RESD 2
.Reserved4:         RESW 1
.IoMapBaseAddress:  RESW 1
.size:
endstruc

struc CpuData 
.SelfPointer:       RESQ 1
.Index:             RESQ 1
.SyscallStack:      RESQ 1
.SyscallRegisters:  RESB SyscallRegisters64.size
.LastAlienPml4:     RESQ 1
.DomainDescriptor:  RESQ 1
.Tss:               RESB Tss.size
.ActiveThread:      RESQ 1
.ActiveProcess:     RESQ 1
.ExceptionContext:  RESQ 1
.size:
endstruc
