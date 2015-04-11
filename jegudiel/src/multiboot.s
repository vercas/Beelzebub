; Copyright (c) 2012 by Lukas Heidemann <lukasheidemann@gmail.com>
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
; NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

section .multiboot
bits 32

MULTIBOOT_FLAG_PAGE_ALIGN       EQU     0x0001
MULTIBOOT_FLAG_MEMORY_INFO      EQU     0x0002
MULTIBOOT_FLAG_VIDEO_MODE       EQU     0x0004
MULTIBOOT_FLAG_AOUT_KLUDGE      EQU     0x0008

MULTIBOOT_MAGIC                 EQU     0x1BADB002
MULTIBOOT_FLAGS                 EQU     MULTIBOOT_FLAG_PAGE_ALIGN | \
                                        MULTIBOOT_FLAG_MEMORY_INFO
MULTIBOOT_CHECKSUM              EQU     -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

multiboot_header:
.Magic:
    dd MULTIBOOT_MAGIC
.Flags:
    dd MULTIBOOT_FLAGS
.Checksum:
    dd MULTIBOOT_CHECKSUM
