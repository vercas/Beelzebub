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

/**
 * Sets up the system to the desired state, which includes:
 * 
 *  - creating an info table (from sources like the multiboot table and ACPI)
 *  - configuring devices (such as the PIC or the IO APIC)
 *  - moving multiboot modules to a known location in physical memory
 *  - loading and mapping the kernel binary
 *  - booting up application processors
 */

/**
 * A barrier used to synchronize BSP and AP entry to the kernel binary. Is one
 * as long main_bsp has not finished and is changed to zero when startup is
 * complete.
 */
extern volatile uint8_t main_entry_barrier;

/**
 * The long mode entry point for the bootstrap processor.
 * 
 * Sets up the system and triggers application processor startup.
 * 
 * Will wait for all application processors to be booted, then release the
 * main_entry_barrier and enter the kernel at its entry point.
 */
void main_bsp(void);

/**
 * The long mode entry point for the application processors.
 * 
 * Sets up the application processor and reports successful startup to the BSP.
 * 
 * When the kernel specified an AP entry point, the AP will spin on the
 * main_entry_barrier and then enter the kernel at said entry point; otherwise
 * it will halt (hlt).
 */
void main_ap(void);
