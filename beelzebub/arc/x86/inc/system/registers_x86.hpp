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
    /**
     *  Flags in the (E|R)FLAGS register.
     */
    enum class FlagsRegisterFlags
        : size_t
    {
        Carry                   = 0x00000001,
        Reserved1               = 0x00000002,
        Parity                  = 0x00000004,
        Reserved2               = 0x00000008,
        Adjust                  = 0x00000010,
        Reserved3               = 0x00000020,
        Zero                    = 0x00000040,
        Sign                    = 0x00000080,
        Trap                    = 0x00000100,
        InterruptEnable         = 0x00000200,
        Direction               = 0x00000400,
        Overflow                = 0x00000800,
        IoPrivilegeLevel        = 0x00003000,
        NestedTask              = 0x00004000,
        Reserved4               = 0x00008000,

        Resume                  = 0x00010000,
        Virtual8086Mode         = 0x00020000,
        AlignmentCheck          = 0x00040000,
        VirtualInterrupt        = 0x00080000,
        VirtualInterruptPending = 0x00100000,
        Cpuid                   = 0x00200000,
    };

    ENUMOPS(FlagsRegisterFlags, size_t)

    /**
     *  Represents the value of a MSR.
     */
    union MsrValue
    {
        struct
        {
            uint32_t Low;
            uint32_t High;
        } Dwords;

        uint64_t Qword;

        //  Little endian, remember!
    };

    /**
     *  <summary>Represents the contents of the IA32_EFER MSR.</summary>
     */
    struct Ia32Efer
    {
        /*  Bit structure:
         *       0       : SYSCALL Enable
         *       8       : Long Mode Enable
         *      10       : Long Mode Active
         *      11       : Execute Disable Bit Enable
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1W( 0, SyscallEnable   )
        BITFIELD_DEFAULT_1W( 8, LongModeEnable  )
        BITFIELD_DEFAULT_1O(10, LongModeActive  )
        BITFIELD_DEFAULT_1W(11, NonExecuteEnable)

        /*  Constructors  */

        /**
         *  Creates a new IA32_EFER structure from the given MSR value.
         */
        __bland inline explicit Ia32Efer(MsrValue const val)
        {
            this->Value = val.Qword;
        }

        /**
         *  Creates a new IA32_EFER structure from the given raw value.
         */
        __bland inline explicit Ia32Efer(uint64_t const val)
        {
            this->Value = val;
        }

        /**
         *  Creates a new IA32_EFER structure with the given flags.
         */
        __bland inline Ia32Efer(bool const syscallEnable
                                     , bool const longModeEnable
                                     , bool const nonExecuteEnable)
        {
            this->Value = (syscallEnable    ? SyscallEnableBit    : 0)
                        | (longModeEnable   ? LongModeEnableBit   : 0)
                        | (nonExecuteEnable ? NonExecuteEnableBit : 0);
        }

        /*  Field(s)  */

    //private:

        uint64_t Value;
    };

    /**
     *  <summary>Represents the contents of the IA32_APIC_BASE MSR.</summary>
     */
    struct Ia32ApicBase
    {
        /*  Bit structure:
         *       0 -   7 : Reserved (must be 0)
         *       8       : Bootstrap Processor
         *       9       : Reserved (must be 0)
         *      10       : Enable x2APIC
         *      11       : Globally Enable LAPIC
         *      12 -  35 : APIC Base
         *      36 -  63 : Reserved (must be 0)
         *
         *  The manual is unclear about the actual maximum size of the APIC base
         *  address. One diagram shows it as bieng 36-bit specifically.
         *  However, the MSR table says "MAXPHYWID".
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1O( 8, BootstrapProcessor)
        BITFIELD_DEFAULT_1W(10, X2ApicEnabled     )
        BITFIELD_DEFAULT_1W(11, GlobalLapicEnabled)

        BITFIELD_DEFAULT_2W(12,  24, paddr_t, ApicBase)

        /*  Constructors  */

        /**
         *  Creates a new IA32_APIC_BASE structure from the given MSR value.
         */
        __bland inline explicit Ia32ApicBase(MsrValue const val)
        {
            this->Value = val.Qword;
        }

        /**
         *  Creates a new IA32_APIC_BASE structure from the given raw value.
         */
        __bland inline explicit Ia32ApicBase(uint64_t const val)
        {
            this->Value = val;
        }

        /*  Field(s)  */

    //private:

        uint64_t Value;
    };
}}
