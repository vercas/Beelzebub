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

// PIT constants
#define PIT_IO_COUNTER      0x40        //< port to access the counter
#define PIT_IO_INIT         0x43        //< port to initialize the PIT
#define PIT_IRQ             0x0         //< PIT IRQ number (in non PCAT_COMPAT mode)
#define PIT_IRQ_PCAT_COMPAT 0x2         //< PIT IRQ number (in PCAT_COMPAT mode)
#define PIT_FREQ_BASE       1193180     //< frequency of the PIT's oscillator in Hz
#define PIT_VECTOR          0x30        //< interrupt vector used by loader

/**
 * Sets a new PIT frequency (in Hz).
 *
 * @param freq the new frequency in Hz
 */
void pit_freq_set(uint16_t freq);

/**
 * Routes the PIT interrupt to the default vector (PIT_VECTOR) on the
 * calling processor using the IO APIC.
 */
void pit_route(void);

/**
 * Masks the PIT interrupt in the IO APIC again.
 */
void pit_mask(void);
