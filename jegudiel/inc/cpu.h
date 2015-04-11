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
 * Result of a call to the CPUID instruction. Stores the value of the four
 * general purpose registers eax, ebx, ecx and edx after invocation.
 */
typedef struct cpu_cpuid_result {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
} cpu_cpuid_result_t;

/**
 * Reads the value of a MSR.
 *
 * @param msr the index of the MSR to read.
 */
uint64_t cpu_msr_read(uint32_t msr);

/**
 * Writes a value to a MSR.
 *
 * @param msr the index of the MSR to write to.
 * @param value the value to write to the MSR.
 */
void cpu_msr_write(uint32_t msr, uint64_t value);

/**
 * Invokes the CPUID instruction for the given code and returns
 * the result in the <result> parameter.
 *
 * @param code the CPUID code (in EAX).
 * @param result output parameter for CPUID result
 */
void cpu_cpuid(uint32_t code, cpu_cpuid_result_t *result);
