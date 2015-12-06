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

#include <system/registers_x86.hpp>

#define MSRFUNC1(name, prettyName, type)                          \
static __forceinline type MCATS2(Get, prettyName)()               \
{                                                                 \
    uint64_t temp = 0;                                            \
    Read(Msr::name, temp);                                        \
    return type(temp);                                            \
}                                                                 \
static __forceinline void MCATS2(Set, prettyName)(type const val) \
{                                                                 \
    Write(Msr::name, val.Value);                                  \
}

#define MSRFUNC2(name, prettyName, type)                          \
static __forceinline type MCATS2(Get, prettyName)()               \
{                                                                 \
    uint64_t temp = 0;                                            \
    Read(Msr::name, temp);                                        \
    return reinterpret_cast<type>(temp);                          \
}                                                                 \
static __forceinline void MCATS2(Set, prettyName)(type const val) \
{                                                                 \
    Write(Msr::name, reinterpret_cast<uint64_t>(val));            \
}

namespace Beelzebub { namespace System
{
    /**
     *  Model-specific registers
     */
    enum class Msr : uint32_t
    {
        //  (L)APIC/x2APIC
        IA32_APIC_BASE      = 0x0000001B,

        //  Extended Feature Enables
        IA32_EFER           = 0xC0000080,

        //  System call Target Address
        IA32_STAR           = 0xC0000081,
        //  Long Mode System Call Target Address
        IA32_LSTAR          = 0xC0000082,
        //  Compatibility Mode System Call Target Address
        IA32_CSTAR          = 0xC0000083,
        //  System call Flag Mask
        IA32_FMASK          = 0xC0000084,

        //  Map of BASE Adddress of FS
        IA32_FS_BASE        = 0xC0000100,
        //  Map of BASE Adddress of GS
        IA32_GS_BASE        = 0xC0000101,
        //  Swap Target of BASE Adddress of GS
        IA32_KERNEL_GS_BASE = 0xC0000102,

        //  Base MSR for x2APIC registers
        IA32_X2APIC_BASE    = 0x00000800,
    };

    /**
     *  Represents a processing unit of the system.
     */
    class Msrs
    {
        /*  Constructor(s)  */

    protected:
        Msrs() = default;

    public:
        Msrs(Msrs const &) = delete;
        Msrs & operator =(Msrs const &) = delete;

        /*  MSRs  */

        static __forceinline MsrValue Read(const Msr reg)
        {
            uint32_t a, d;

            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            return {{d, a}};
        }

        static __forceinline uint64_t Read64(const Msr reg)
        {
            uint32_t a, d;

            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            return (uint64_t)a | ((uint64_t)d << 32);
        }

        static __forceinline void Read(const Msr reg, uint32_t & a, uint32_t & d)
        {
            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );
        }

        static __forceinline void Read(const Msr reg, uint64_t & val)
        {
            uint32_t a, d;

            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            val = (uint64_t)a | ((uint64_t)d << 32);
        }

        static __forceinline void Write(const Msr reg, const MsrValue val)
        {
            asm volatile ( "wrmsr \n\t"
                         : 
                         : "c" (reg), "a" (val.Dwords.Low), "d" (val.Dwords.High) );
        }

        static __forceinline void Write(const Msr reg, const uint32_t a, const uint32_t d)
        {
            asm volatile ( "wrmsr \n\t"
                         : 
                         : "c" (reg), "a" (a), "d" (d) );
        }

        static __forceinline void Write(const Msr reg, const uint64_t val)
        {
            register uint32_t a asm("eax") = (uint32_t)val;
            register uint32_t d asm("edx") = (uint32_t)(val >> 32);

            asm volatile ( "wrmsr \n\t"
                         : 
                         : "c" (reg), "a" (a), "d" (d) );
        }

        MSRFUNC1(IA32_EFER     , Efer    , Ia32Efer    )
        MSRFUNC1(IA32_APIC_BASE, ApicBase, Ia32ApicBase)
        MSRFUNC1(IA32_STAR     , Star    , Ia32Star    )
        MSRFUNC1(IA32_LSTAR    , Lstar   , Ia32Lstar   )
        MSRFUNC1(IA32_CSTAR    , Cstar   , Ia32Cstar   )
        MSRFUNC1(IA32_FMASK    , Fmask   , Ia32Fmask   )

        /*  Shortcuts  */

        static __forceinline void EnableNxBit()
        {
            const Msr reg = Msr::IA32_EFER;

            asm volatile ( "rdmsr           \n\t"
                           "or $2048, %%eax \n\t" //  That be bit 11.
                           "wrmsr           \n\t"
                         :
                         : "c" (reg)
                         : "eax", "edx" );
        }
    };
}}
