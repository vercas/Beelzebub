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

#include <utils/bitfields.hpp>

namespace Beelzebub { namespace System
{
    enum class IdtGateType : uint8_t
    {
        Unused00        =  0,
        Unused01        =  1,
        Unused02        =  2,
        Unused03        =  3,
        Unused04        =  4,
        TaskGate        =  5,
        InterruptGate16 =  6,
        TrapGate16      =  7,
        Unused08        =  8,
        Unused09        =  9,
        Unused10        = 10,
        Unused11        = 11,
        Unused12        = 12,
        Unused13        = 13,
        InterruptGate   = 14,
        TrapGate        = 15,
    };

#include <system/idt_gate.inc>

    class Idt
    {
    public:

        /*  Constructor(s)  */

        Idt() = default;

        Idt(Idt const &) = delete;
        Idt & operator =(Idt const &) = delete;

        /*  Field(s)  */

        IdtGate Entries[256];
    };

    /**
     *  <summary>Structure of the GDTR.</summary>
     */
    struct IdtRegister
    {
        /*  Field(s)  */

        uint16_t Size;
        Idt * Pointer;

        /*  Load & Store  */

        __bland inline void Activate()
        {
            asm volatile ( "lidt (%[ptr]) \n\t" : : [ptr]"r"(this) );
        }

        static __bland inline IdtRegister Retrieve()
        {
            IdtRegister res;

            asm volatile ( "sidt %[ptr] \n\t" : : [ptr]"m"(res) );

            return res;
        }
    } __packed;
}}
