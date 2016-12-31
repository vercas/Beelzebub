/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#pragma once

#include <beel/structs.kernel.common.h>

#ifndef __ASSEMBLER__
STRUCT(ExceptionContext)
{
    uint64_t RBX, RCX, RBP;
    uint64_t R12, R13, R14, R15, RSP;

    uintptr_t ResumePointer, SwapPointer;
    //  The `ResumePointer` is used by interrupt handlers, which restores
    //  registers and sets the context as handling. The `SwapPointer` is used
    //  to do all that manually.

    uintptr_t ReturnAddress;

    Exception const * Payload;
    ExceptionContext * Previous;
    ExceptionStatus Status;
} __packed;
#else
.struct 0
FIELDT(ExceptionContext, RBX           , uint64_t   )
FIELDT(ExceptionContext, RCX           , uint64_t   )
FIELDT(ExceptionContext, RBP           , uint64_t   )
FIELDT(ExceptionContext, R12           , uint64_t   )
FIELDT(ExceptionContext, R13           , uint64_t   )
FIELDT(ExceptionContext, R14           , uint64_t   )
FIELDT(ExceptionContext, R15           , uint64_t   )
FIELDT(ExceptionContext, RSP           , uint64_t   )
FIELDT(ExceptionContext, ResumePointer , uintptr_t  )
FIELDT(ExceptionContext, SwapPointer   , uintptr_t  )
FIELDT(ExceptionContext, ReturnAddress , uintptr_t  )
FIELDT(ExceptionContext, Payload       , uintptr_t  )
FIELDT(ExceptionContext, Previous      , uintptr_t  )
FIELDT(ExceptionContext, Status        , uintptr_t  )
ExceptionContext_size:
#endif

/**
 *  A unit of isolation.
 */
#ifndef __ASSEMBLER__
STRUCT(Process)
{
    uint16_t Id;
};
#endif

/**
 *  A unit of execution.
 */
#ifndef __ASSEMBLER__
STRUCT(Thread)
{
    Process * const Owner;

    ExceptionContext * XContext;
    Exception X;
};
#endif

#ifndef __ASSEMBLER__
STRUCT(SyscallRegisters64)
{
    uint64_t R15;
    uint64_t R14;
    uint64_t R13;
    uint64_t R12;
    uint64_t RFLAGS;    //  R11
    uint64_t R10;
    uint64_t R9;
    uint64_t R8;
    uint64_t RSI;
    uint64_t RDI;
    uint64_t RSP;
    uint64_t RBP;
    uint64_t RDX;
    uint64_t RIP;       //  RCX
    uint64_t RBX;
    uint64_t RAX;
};
#else
.struct 0
FIELDT(SyscallRegisters64, R15      , uint64_t)
FIELDT(SyscallRegisters64, R14      , uint64_t)
FIELDT(SyscallRegisters64, R13      , uint64_t)
FIELDT(SyscallRegisters64, R12      , uint64_t)
FIELDT(SyscallRegisters64, RFLAGS   , uint64_t)
FIELDT(SyscallRegisters64, R10      , uint64_t)
FIELDT(SyscallRegisters64, R9       , uint64_t)
FIELDT(SyscallRegisters64, R8       , uint64_t)
FIELDT(SyscallRegisters64, RSI      , uint64_t)
FIELDT(SyscallRegisters64, RDI      , uint64_t)
FIELDT(SyscallRegisters64, RSP      , uint64_t)
FIELDT(SyscallRegisters64, RBP      , uint64_t)
FIELDT(SyscallRegisters64, RDX      , uint64_t)
FIELDT(SyscallRegisters64, RIP      , uint64_t)
FIELDT(SyscallRegisters64, RBX      , uint64_t)
FIELDT(SyscallRegisters64, RAX      , uint64_t)
SyscallRegisters64_size:
#define SIZE_OF_SyscallRegisters64 SyscallRegisters64_size
#endif

#ifndef __ASSEMBLER__
STRUCT(SyscallRegisters32)
{
    uint32_t ESI;
    uint32_t EDI;
    uint32_t ESP;
    uint32_t EBP;
    uint32_t EDX;
    uint32_t ECX;
    uint32_t EBX;
    uint32_t EAX;
};
#else
.struct 0
FIELDT(SyscallRegisters32, ESI, uint32_t)
FIELDT(SyscallRegisters32, EDI, uint32_t)
FIELDT(SyscallRegisters32, ESP, uint32_t)
FIELDT(SyscallRegisters32, EBP, uint32_t)
FIELDT(SyscallRegisters32, EDX, uint32_t)
FIELDT(SyscallRegisters32, ECX, uint32_t)
FIELDT(SyscallRegisters32, EBX, uint32_t)
FIELDT(SyscallRegisters32, EAX, uint32_t)
SyscallRegisters32_size:
#define SIZE_OF_SyscallRegisters32 SyscallRegisters32_size
#endif

/**
 *  The data available to an individual CPU core.
 */
#ifndef __ASSEMBLER__
STRUCT(CpuData)
{
    CpuData * SelfPointer;
    size_t Index;
    uint32_t LapicId;
    uint32_t Padding1;

    uintptr_t SyscallStack, SyscallUserlandStack;
    SyscallRegisters64 SyscallRegisters;

    Thread * ActiveThread;
    Process * ActiveProcess;
} __aligned(128);
#else
.struct 0
FIELDT(CpuData, SelfPointer         , uintptr_t         )
FIELDT(CpuData, Index               , size_t            )
FIELDT(CpuData, LapicId             , uint32_t          )
FIELDT(CpuData, Padding1            , uint32_t          )
FIELDT(CpuData, ActiveThread        , uintptr_t         )
FIELDT(CpuData, ActiveProcess       , uintptr_t         )
CpuData_size:
#define SIZE_OF_CpuData CpuData_size
#endif
