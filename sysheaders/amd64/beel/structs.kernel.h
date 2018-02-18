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

#ifdef __BEELZEBUB_KERNEL
    #define __BASE(NAME) MCATS(NAME, Base)
#else
    #define __BASE(NAME) NAME
#endif

__NAMESPACE_BEGIN

#ifndef __ASSEMBLER__
__STRUCT(ExceptionContext)
{
    uint64_t RBX, RCX, RBP;
    uint64_t R12, R13, R14, R15, RSP;

    uintptr_t ResumePointer, SwapPointer;
    //  The `ResumePointer` is used by interrupt handlers, which restores
    //  registers and sets the context as handling. The `SwapPointer` is used
    //  to do all that manually.

    uintptr_t ReturnAddress;

    BeException const * Payload;
    BeExceptionContext * Previous;
    BeExceptionContextStatus Status;
};
#else
.struct 0
FIELDT(BeExceptionContext, RBX           , uint64_t   )
FIELDT(BeExceptionContext, RCX           , uint64_t   )
FIELDT(BeExceptionContext, RBP           , uint64_t   )
FIELDT(BeExceptionContext, R12           , uint64_t   )
FIELDT(BeExceptionContext, R13           , uint64_t   )
FIELDT(BeExceptionContext, R14           , uint64_t   )
FIELDT(BeExceptionContext, R15           , uint64_t   )
FIELDT(BeExceptionContext, RSP           , uint64_t   )
FIELDT(BeExceptionContext, ResumePointer , uintptr_t  )
FIELDT(BeExceptionContext, SwapPointer   , uintptr_t  )
FIELDT(BeExceptionContext, ReturnAddress , uintptr_t  )
FIELDT(BeExceptionContext, Payload       , uintptr_t  )
FIELDT(BeExceptionContext, Previous      , uintptr_t  )
FIELDT(BeExceptionContext, Status        , uintptr_t  )
BeExceptionContext_size:
#endif

/**
 *  A unit of isolation.
 */
#ifndef __ASSEMBLER__
__STRUCT(__BASE(Process))
{
#ifdef __BEELZEBUB__SOURCE_CXX
protected:
    /*  Constructor(s)  */

    inline __BASE(Process)(uint16_t const id) : Id( id) { }

public:
#endif

    uint16_t const Id;
};
#endif

/**
 *  A unit of execution.
 */
#ifndef __ASSEMBLER__
__STRUCT(__BASE(Thread))
{
    __BASE(BeProcess) * const Owner;

    BeExceptionContext * XContext;
    BeException X;
};
#endif

#ifndef __ASSEMBLER__
__STRUCT(GeneralRegisters64)
{
    uint64_t DS;

    uint64_t R15;
    uint64_t R14;
    uint64_t R13;
    uint64_t R12;
    uint64_t R11;
    uint64_t R10;
    uint64_t R9;
    uint64_t R8;
    uint64_t RSI;
    uint64_t RDI;
    uint64_t RBP;
    uint64_t RDX;
    uint64_t RBX;
    uint64_t RAX;
    
    uint64_t RCX;

    uint64_t ErrorCode;

    uint64_t RIP;
    uint64_t CS;
    uint64_t RFLAGS;
    uint64_t RSP;
    uint64_t SS;
};
#else
.struct 0
FIELDT(BeGeneralRegisters64, DS       , uint64_t)
FIELDT(BeGeneralRegisters64, R15      , uint64_t)
FIELDT(BeGeneralRegisters64, R14      , uint64_t)
FIELDT(BeGeneralRegisters64, R13      , uint64_t)
FIELDT(BeGeneralRegisters64, R12      , uint64_t)
FIELDT(BeGeneralRegisters64, R11      , uint64_t)
FIELDT(BeGeneralRegisters64, R10      , uint64_t)
FIELDT(BeGeneralRegisters64, R9       , uint64_t)
FIELDT(BeGeneralRegisters64, R8       , uint64_t)
FIELDT(BeGeneralRegisters64, RSI      , uint64_t)
FIELDT(BeGeneralRegisters64, RDI      , uint64_t)
FIELDT(BeGeneralRegisters64, RBP      , uint64_t)
FIELDT(BeGeneralRegisters64, RDX      , uint64_t)
FIELDT(BeGeneralRegisters64, RBX      , uint64_t)
FIELDT(BeGeneralRegisters64, RAX      , uint64_t)
FIELDT(BeGeneralRegisters64, RCX      , uint64_t)
FIELDT(BeGeneralRegisters64, ErrorCode, uint64_t)
FIELDT(BeGeneralRegisters64, RIP      , uint64_t)
FIELDT(BeGeneralRegisters64, CS       , uint64_t)
FIELDT(BeGeneralRegisters64, RFLAGS   , uint64_t)
FIELDT(BeGeneralRegisters64, RSP      , uint64_t)
FIELDT(BeGeneralRegisters64, SS       , uint64_t)
BeGeneralRegisters64_size:
#define SIZE_OF_BeGeneralRegisters64 BeGeneralRegisters64_size
#endif

#ifndef __ASSEMBLER__
__STRUCT(GeneralRegisters32)
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
FIELDT(BeGeneralRegisters32, ESI, uint32_t)
FIELDT(BeGeneralRegisters32, EDI, uint32_t)
FIELDT(BeGeneralRegisters32, ESP, uint32_t)
FIELDT(BeGeneralRegisters32, EBP, uint32_t)
FIELDT(BeGeneralRegisters32, EDX, uint32_t)
FIELDT(BeGeneralRegisters32, ECX, uint32_t)
FIELDT(BeGeneralRegisters32, EBX, uint32_t)
FIELDT(BeGeneralRegisters32, EAX, uint32_t)
BeGeneralRegisters32_size:
#define SIZE_OF_BeGeneralRegisters32 BeGeneralRegisters32_size
#endif

/**
 *  The data available to an individual CPU core.
 */
#ifndef __ASSEMBLER__
__STRUCT(CpuData)
{
    BeCpuData * SelfPointer;
    size_t Index;
    uint32_t LapicId;
    uint32_t Padding1;

    uintptr_t SyscallStack, SyscallUserlandStack;
    BeGeneralRegisters64 GeneralRegisters;

    BeThread * ActiveThread;
    BeProcess * ActiveProcess;
} __aligned(128);
#else
.struct 0
FIELDT(BeCpuData, SelfPointer         , uintptr_t         )
FIELDT(BeCpuData, Index               , size_t            )
FIELDT(BeCpuData, LapicId             , uint32_t          )
FIELDT(BeCpuData, Padding1            , uint32_t          )
FIELDT(BeCpuData, ActiveThread        , uintptr_t         )
FIELDT(BeCpuData, ActiveProcess       , uintptr_t         )
BeCpuData_size:
#define SIZE_OF_BeCpuData BeCpuData_size
#endif

__NAMESPACE_END
