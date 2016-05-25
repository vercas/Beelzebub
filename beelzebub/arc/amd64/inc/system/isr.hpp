/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


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

#include <metaprogramming.h>

namespace Beelzebub { namespace System
{
    /**
     *  The state of the system before an interrupt was raised, which
     *  that can be used and manipulated by the ISR.
     */
    struct IsrState
    {
        /*  Field(s)  */

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
    } __packed;

    /**
     *  The partial state of the system before an interrupt was raised, which
     *  is not meant to be manipulated by the ISR.
     */
    struct IsrStatePartial
    {
        /*  Field(s)  */

        uint64_t DS;

        uint64_t R11;
        uint64_t R10;
        uint64_t R9;
        uint64_t R8;
        uint64_t RSI;
        uint64_t RDI;
        uint64_t RBP;
        uint64_t RDX;
        uint64_t RAX;
        
        uint64_t RCX;

        uint64_t ErrorCode;

        uint64_t RIP;
        uint64_t CS;
        uint64_t RFLAGS;
        uint64_t RSP;
        uint64_t SS;
    } __packed;

    /**
     *  The stub entered by each syscall.
     */
    union IsrStub
    {
        /*  Static(s)  */

        static constexpr uint8_t PushRcxValue = 0x51;

        /*  Properties  */

        inline bool ProvidesErrorCode()
        {
            return this->Bytes[0] == PushRcxValue;
        }

        inline void * GetJumpBase()
        {
            if (this->ProvidesErrorCode())
                return &(this->WithErrorCode.JumpOffset) + 1;
            else
                return &(this->NoErrorCode.JumpOffset) + 1;
        }

        inline void * GetJumpTarget()
        {
            if (this->ProvidesErrorCode())
                return reinterpret_cast<uint8_t *>(&(this->WithErrorCode.JumpOffset) + 1) + this->WithErrorCode.JumpOffset;
            else
                return reinterpret_cast<uint8_t *>(&(this->NoErrorCode.JumpOffset) + 1) + this->NoErrorCode.JumpOffset;
        }

        inline IsrStub & SetJumpTarget(void const * const val)
        {
            if (this->ProvidesErrorCode())
                this->WithErrorCode.JumpOffset =
                    (int32_t)reinterpret_cast<int64_t>(
                        reinterpret_cast<intptr_t>(val)
                        - reinterpret_cast<intptr_t>(&(this->WithErrorCode.JumpOffset) + 1));
            else
                this->NoErrorCode.JumpOffset =
                    (int32_t)reinterpret_cast<int64_t>(
                        reinterpret_cast<intptr_t>(val)
                        - reinterpret_cast<intptr_t>(&(this->NoErrorCode.JumpOffset) + 1));

            return *this;
        }

        /*  Field(s)  */

        uint8_t Bytes[16];

        struct
        {
            uint8_t PushRcx;
            uint8_t MovCl;
            uint8_t Vector;
            uint8_t Jump;
            int32_t JumpOffset;
        } __packed WithErrorCode;

        struct
        {
            uint8_t PushImmediate;
            uint8_t DummyErrorCode;
            uint8_t PushRcx;
            uint8_t MovCl;
            uint8_t Vector;
            uint8_t Jump;
            int32_t JumpOffset;
        } __packed NoErrorCode;
    };

    static_assert(sizeof(IsrStub) == 16);

    __extern IsrStub IsrStubsBegin, IsrStubsEnd;
}}
