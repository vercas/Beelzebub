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

#include <beel/handles.h>
#include <beel/enums.kernel.h>

#define BEELZEBUB_MESSAGE_SIZE (64)
#define BEELZEBUB_MESSAGE_DWORDS (5)

__NAMESPACE_BEGIN

#ifndef __ASSEMBLER__
    __STRUCT(Message)
    {
    #ifdef __BEELZEBUB__SOURCE_CXX
        static constexpr unsigned short Size = BEELZEBUB_MESSAGE_SIZE;
        static constexpr unsigned short DwordCount = BEELZEBUB_MESSAGE_DWORDS;
    #endif

        BeHandle Destination, Source;

        __extension__ union
        {
            uint64_t Flags;

            __extension__ struct
            {
                int32_t Type;
                uint32_t FlagsLow;
            };
        };

        __extension__ union
        {
            BeHandle Handles[BEELZEBUB_MESSAGE_DWORDS];  //  Handles (double words)
            uint64_t D[BEELZEBUB_MESSAGE_DWORDS    ];  //  Double words
            uint32_t W[BEELZEBUB_MESSAGE_DWORDS * 2];  //  Words
            uint16_t H[BEELZEBUB_MESSAGE_DWORDS * 4];  //  Half words
            uint8_t  B[BEELZEBUB_MESSAGE_DWORDS * 8];  //  Bytes
        };
    };
#else
#endif

#ifdef __BEELZEBUB__SOURCE_CXX
    static_assert(sizeof(Message) == Message::Size, "IPC message struct should be exactly 64 bytes in size!");

    #undef BEELZEBUB_MESSAGE_DWORDS
    #undef BEELZEBUB_MESSAGE_SIZE
#endif

#ifndef __ASSEMBLER__
__STRUCT(MemoryAccessViolationData)
{
    //  These are ordered by size, descending.

    paddr_t PhysicalAddress;
    vaddr_t Address;
    //  Physical address may be larger (PAE).

    MemoryLocationFlags PageFlags;
    MemoryAccessType AccessType;

#ifdef __BEELZEBUB__SOURCE_CXX
    inline MemoryAccessViolationData()
        : PhysicalAddress( 0)
        , Address(nullvaddr)
        , PageFlags()
        , AccessType()
    {

    }
#endif
};
// #else
// .struct 0
// FIELDT(BeMemoryAccessViolationData, PhysicalAddress, paddr_t    )
// FIELDT(BeMemoryAccessViolationData, Address        , vaddr_t    )
// FIELDT(BeMemoryAccessViolationData, PageFlags      , uint16_t   )
// FIELDT(BeMemoryAccessViolationData, AccessType     , uint8_t    )
// BeMemoryAccessViolationData_size:
#endif

#ifndef __ASSEMBLER__
__STRUCT(UnitTestFailureData)
{
    char const * FileName;
    size_t Line;
};
// #else
// .struct 0
// FIELDT(BeUnitTestFailureData, FileName, intptr_t )
// FIELDT(BeUnitTestFailureData, Line    , size_t   )
// BeUnitTestFailureData_size:
#endif

#ifndef __ASSEMBLER__
__STRUCT(Exception)
{
    ExceptionType Type;
    uintptr_t InstructionPointer;
    uintptr_t StackPointer;

    __extension__ union
    {
        MemoryAccessViolationData MemoryAccessViolation;
        UnitTestFailureData UnitTestFailure;
    };

#ifdef __BEELZEBUB__SOURCE_CXX
    inline Exception()
        : Type(ExceptionType::None)
        , InstructionPointer()
        , StackPointer()
        , MemoryAccessViolation()
    {

    }
#endif
};
#endif

__NAMESPACE_END
