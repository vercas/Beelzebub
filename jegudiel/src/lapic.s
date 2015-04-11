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

section .text
bits 64

global lapic_timer_calibrate_worker
global lapic_timer_calibrate_handler
global lapic_timer_wait_handler
global lapic_timer_wait_worker
extern lapic_timer_update
extern lapic_register_read
extern lapic_eoi

; Auxiliary function for LAPIC timer calibration.
;
; Enables interrupts, waits for the calibration to complete, then
; returns the result (counter ticks per second) in RAX.
;
; An assembly function is required, as the interrupt handler does
; not store nor restore the registers. The current phase of calibration
; is stored in rdx, the result in rax.
;
; Requires the PIT to be configured to 100Hz and the IRQ to be
; routed to the lapic_timer_calibrate_handler ISR using the IO APIC.
;
lapic_timer_calibrate_worker:
	xor rdx, rdx						; Set phase to zero
	sti									; Start interrupts

.wait:
	cmp rdx, 2							; Wait until in phase 2
	jl .wait

	cli									; Disable interrupts
	ret

; Auxiliary function for LAPIC timer calibration.
;
; Handles the PIT timer IRQ and measures the ticks of the LAPIC timer
; that elapse in between two PIT timer ticks. As it is assumed that
; the PIT runs at 100Hz, the result is multiplied with 100 and then
; stored in rax when finished.
;
; Defines two phases, which are stored in the rdx register. Assumes
; that rdx is zero when the ISR is called the first time.
;
; First phase: The LAPIC timer is started, with an initial count set
; to the maximum value and masked interrupts in one-shot mode.
;
; Second phase: The LAPIC timer's tick count is read and stored as
; a result.
;
; All subsequent calls of this ISR will have no effect.
lapic_timer_calibrate_handler:
	cli                                ; Disable interrupts
	cmp rdx, 0                         ; First phase?
	je .phase_first
	cmp rdx, 1                         ; Second phase?
	je .phase_second
	jmp .end							; Already complete

.phase_first:							; First phase
    push rax
	push rdx
	mov rdi, 0xFFFFFFFF					; Max initial count
	mov rsi, 0							; Some vector
	mov rdx, 1							; Mask
	mov r8, 0							; One-shot
	call lapic_timer_update
	pop rdx
	pop rax
	jmp .end

.phase_second:							; Second phase
	push rdx
	mov rdi, 0x39						; Current timer count
	call lapic_register_read
	pop rdx

	mov rbx, rax						; Subtract timer count from
	mov rax, 0xFFFFFFFF					; initial timer count to
	sub rax, rbx						; get the difference

	mov rbx, 100						; Multiply with 100 to get
	push rdx							; timer ticks per second
	mul ebx
	pop rdx

.end:
	inc rdx								; Next phase
	push rax
	push rdx
	call lapic_eoi						; EOI
	pop rdx
	pop rax
	sti                                 ; Enable interrupts
	iretq								; Return from ISR

; Enables interrupts, waits for a barrier, then disables interrupts again.
lapic_timer_wait_worker:
    xor r15, r15            ; Barrier in R15

    sti
.wait:
    cmp r15, 1
    jne .wait

    cli
    ret

; ISR for lapic_timer_wait that sets the lapic_timer_wait_barrier on IRQ.
lapic_timer_wait_handler:
    call lapic_eoi
    mov r15, 1
    iretq
