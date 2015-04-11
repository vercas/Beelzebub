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

// Pin polarity
#define APIC_POLARITY_HIGH  0
#define APIC_POLARITY_LOW   1        

// Delivery mode
#define APIC_DELIVERY_FIXED     0b000
#define APIC_DELIVERY_LOW_PRIO  0b001
#define APIC_DELIVERY_SMI       0b010
#define APIC_DELIVERY_NMI       0b100
#define APIC_DELIVERY_INIT      0b101
#define APIC_DELIVERY_STARTUP   0b110

// Destination shorthand
#define APIC_SHORT_NONE         0b00
#define APIC_SHORT_SELF         0b01
#define APIC_SHORT_ALL_INCL     0b10
#define APIC_SHORT_ALL_EXCL     0b11

// Interrupt level
#define APIC_LEVEL_DEASSERT     0
#define APIC_LEVEL_ASSERT       1

// Trigger mode
#define APIC_TRIGGER_EDGE       0
#define APIC_TRIGGER_LEVEL      1

// Destination mode
#define APIC_MODE_PHYSICAL      0
#define APIC_MODE_LOGICAL       1

