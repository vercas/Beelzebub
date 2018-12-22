; You can't copyright a multiboot header.
; Also, I changed it long ago.

section .multiboot
bits 32

MULTIBOOT_FLAG_PAGE_ALIGN       EQU     0x0001
MULTIBOOT_FLAG_MEMORY_INFO      EQU     0x0002
MULTIBOOT_FLAG_VIDEO_MODE       EQU     0x0004
MULTIBOOT_FLAG_AOUT_KLUDGE      EQU     0x0008

MULTIBOOT_MAGIC                 EQU     0x1BADB002
MULTIBOOT_FLAGS                 EQU     MULTIBOOT_FLAG_PAGE_ALIGN | \
                                        MULTIBOOT_FLAG_VIDEO_MODE | \
                                        MULTIBOOT_FLAG_MEMORY_INFO
MULTIBOOT_CHECKSUM              EQU     -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

multiboot_header:
.Magic:
    dd MULTIBOOT_MAGIC
.Flags:
    dd MULTIBOOT_FLAGS
.Checksum:
    dd MULTIBOOT_CHECKSUM
.UNUSED:
	dd 0	;	Header address?
	dd 0	;	Load address?
	dd 0	;	Load "end" address..?
	dd 0	;	No idea.
	dd 0	;	"Entry" address..? o.0
.VideoMode:
	dd 0
	dd 0
	dd 0
	dd 0
