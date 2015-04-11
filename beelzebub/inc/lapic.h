/**
 * Copyright (c) 2012 by Lukas Heidemann <lukasheidemann@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <stdint.h>

// register indices
#define LAPIC_REG_ID            0x02    //< LAPIC ID
#define LAPIC_REG_EOI           0x0B    //< End of Interrupt
#define LAPIC_REG_ICR_LOW       0x30    //< Interrupt Command Register (lower DWORD)
#define LAPIC_REG_ICR_HIGH      0x31    //< Interrupt Command Register (upper DWORD)
#define LAPIC_REG_TIMER         0x32    //< Local: Timer
#define LAPIC_REG_TIMER_INIT    0x38    //< Timer: Initial Count
#define LAPIC_REG_TIMER_CUR     0x39    //< Timer: Current Count
#define LAPIC_REG_TIMER_DIV     0x3E    //< Timer: Divisor

// x2APIC related
#define LAPIC_MSR_REGS          0x800

#define lapic_id() lapic_register_read(LAPIC_REG_ID)
#define lapic_eoi() lapic_register_write(LAPIC_REG_EOI, 0)

/**
 * Reads a register of the CPU's LAPIC, given its <index>.
 *
 * Uses the LAPIC address stored in the info tables.
 * Behavior is undefined when called for an invalid register.
 *
 * @param index the index of the register
 * @return value of the register
 */
uint32_t lapic_register_read(uint16_t index);

/**
 * Writes a <value> to a register of the CPU's LAPIC, given its <index>.
 *
 * Uses the LAPIC address stored in the info tables.
 * Behavior is undefined when called for an invalid register.
 *
 * @param index the index of the register to write
 * @param value the new value of the register
 */
void lapic_register_write(uint16_t index, uint32_t value);
