#pragma once

#include <terminals/base.hpp>
#include <handles.h>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

//  Creates a getter for bit-based properties.
#define BITPROPRO(name)                                      \
__bland __forceinline bool MCATS2(Get, name)() const         \
{                                                            \
    return 0 != (this->Value & MCATS2(name, Bit));           \
}

//  Creates a getter and setter for bit-based properties.
#define BITPROPRW(name)                                      \
__bland __forceinline bool MCATS2(Get, name)() const         \
{                                                            \
    return 0 != (this->Value & MCATS2(name, Bit));           \
}                                                            \
__bland __forceinline void MCATS2(Set, name)(const bool val) \
{                                                            \
    if (val)                                                 \
        this->Value |=  MCATS2(name, Bit);                   \
    else                                                     \
        this->Value &= ~MCATS2(name, Bit);                   \
}

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
     * Represents the contents of the IA32_EFER MSR.
     */
    struct Ia32Efer
    {
        /*  Bit structure:
         *       0       : SYSCALL Enable
         *       8       : Long Mode Enable
         *      10       : Long Mode Active
         *      11       : Execute Disable Bit Enable
         */

        static const uint64_t SyscallEnableBit    = 1ULL <<  0;
        static const uint64_t LongModeEnableBit   = 1ULL <<  8;
        static const uint64_t LongModeActiveBit   = 1ULL << 10;
        static const uint64_t NonExecuteEnableBit = 1ULL << 11;

        /*  Constructors  */

        /**
         *  Creates a new IA32_EFER structure from the given MSR value.
         */
        __bland __forceinline Ia32Efer(const MsrValue val)
        {
            this->Value = val.Qword;
        }

        /**
         *  Creates a new IA32_EFER structure from the given raw value.
         */
        __bland __forceinline Ia32Efer(const uint64_t val)
        {
            this->Value = val;
        }

        /**
         *  Creates a new IA32_EFER structure with the given flags.
         */
        __bland __forceinline Ia32Efer(const bool syscallEnable
                                     , const bool longModeEnable
                                     , const bool nonExecuteEnable)
        {
            this->Value = (syscallEnable    ? SyscallEnableBit    : 0)
                        | (longModeEnable   ? LongModeEnableBit   : 0)
                        | (nonExecuteEnable ? NonExecuteEnableBit : 0);
        }

        /*  Properties  */

        BITPROPRW(SyscallEnable)
        BITPROPRW(LongModeEnable)
        BITPROPRO(LongModeActive)
        BITPROPRW(NonExecuteEnable)

        /*  Operators  */

        /**
         *  Gets the value of a bit.
         */
        __bland __forceinline bool operator[](const uint8_t bit) const
        {
            return 0 != (this->Value & (1 << bit));
        }

        /*  Debug  */

        __cold __bland TerminalWriteResult PrintToTerminal(TerminalBase * const term) const;

        /*  Field(s)  */

    //private:

        uint64_t Value;
    };
}}
