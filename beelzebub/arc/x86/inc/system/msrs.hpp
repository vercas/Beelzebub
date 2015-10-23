#pragma once

#include <system/registers_x86.hpp>

#define MSRFUNC1(name, prettyName, type)                                  \
static __bland __forceinline type MCATS2(Get, prettyName)()               \
{                                                                         \
    uint64_t temp = 0;                                                    \
    Read(Msr::name, temp);                                             \
    return type(temp);                                                    \
}                                                                         \
static __bland __forceinline void MCATS2(Set, prettyName)(const type val) \
{                                                                         \
    Write(Msr::name, val.Value);                                       \
}

namespace Beelzebub { namespace System
{
    /**
     *  Model-specific registers
     */
    enum class Msr : uint32_t
    {
        //  Extended Feature Enables
        IA32_EFER           = 0xC0000080,

        //  System call Target Address
        IA32_STAR           = 0xC0000081,
        //  Long Mode System call Target Address
        IA32_LSTAR          = 0xC0000082,
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
    public:

        /*  MSRs  */

        static __bland __forceinline MsrValue Read(const Msr reg)
        {
            uint32_t a, d;

            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            return {{d, a}};
        }

        static __bland __forceinline uint64_t Read64(const Msr reg)
        {
            uint32_t a, d;

            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            return (uint64_t)a | ((uint64_t)d << 32);
        }

        static __bland __forceinline void Read(const Msr reg, uint32_t & a, uint32_t & d)
        {
            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );
        }

        static __bland __forceinline void Read(const Msr reg, uint64_t & val)
        {
            uint32_t a, d;

            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            val = (uint64_t)a | ((uint64_t)d << 32);
        }

        static __bland __forceinline void Write(const Msr reg, const MsrValue val)
        {
            asm volatile ( "wrmsr \n\t"
                         : 
                         : "c" (reg), "a" (val.Dwords.Low), "d" (val.Dwords.High) );
        }

        static __bland __forceinline void Write(const Msr reg, const uint32_t a, const uint32_t d)
        {
            asm volatile ( "wrmsr \n\t"
                         : 
                         : "c" (reg), "a" (a), "d" (d) );
        }

        static __bland __forceinline void Write(const Msr reg, const uint64_t val)
        {
            register uint32_t a asm("eax") = (uint32_t)val;
            register uint32_t d asm("edx") = (uint32_t)(val >> 32);

            asm volatile ( "wrmsr \n\t"
                         : 
                         : "c" (reg), "a" (a), "d" (d) );
        }

        MSRFUNC1(IA32_EFER, EFER, Ia32Efer)

        /*  Shortcuts  */

        static __bland __forceinline void EnableNxBit()
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
