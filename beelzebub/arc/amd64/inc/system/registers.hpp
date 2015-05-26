#pragma once

//#include <arc/system/cpu.hpp>
#include <arc/memory/paging.hpp>
#include <terminals/base.hpp>
#include <handles.h>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Memory::Paging;
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
     * Represents the contents of the CR0 register.
     */
    struct Cr0
    {
        /*  Bit structure:
         *       0       : Protected Mode Enable
         *       1       : Monitor Co-processor
         *       2       : Emulation
         *       3       : Task Switched
         *       4       : Extension Type
         *       5       : Numeric Error
         *      16       : Write Protect
         *      18       : Alignment Mask
         *      29       : Not-Write Through
         *      30       : Cache Disable
         *      31       : Paging
         */

        static const uint64_t ProtectedModeEnableBit = 1ULL <<  0;
        static const uint64_t MonitorCoprocessorBit  = 1ULL <<  1;
        static const uint64_t EmulationBit           = 1ULL <<  2;
        static const uint64_t TaskSwitchedBit        = 1ULL <<  3;
        static const uint64_t ExtensionTypeBit       = 1ULL <<  4;
        static const uint64_t NumericErrorBit        = 1ULL <<  5;
        static const uint64_t WriteProtectBit        = 1ULL << 16;
        static const uint64_t AlignmentMaskBit       = 1ULL << 18;
        static const uint64_t NotWriteThroughBit     = 1ULL << 29;
        static const uint64_t CacheDisableBit        = 1ULL << 30;
        static const uint64_t PagingBit              = 1ULL << 31;

        /*  Constructors  */

        /**
         *  Creates a new CR0 structure from the given raw value.
         */
        __bland __forceinline Cr0(const uint64_t val)
        {
            this->Value = val;
        }

        /**
         *  Creates a new CR0 structure with the given flags.
         */
        __bland __forceinline Cr0(const bool protectedModeEnable
                                , const bool monitorCoprocessor
                                , const bool emulation
                                , const bool taskSwitched
                                , const bool extensionType
                                , const bool numericError
                                , const bool writeProtect
                                , const bool alignmentMask
                                , const bool notWriteThrough
                                , const bool cacheDisable
                                , const bool paging)
        {
            this->Value = (protectedModeEnable ? ProtectedModeEnableBit : 0)
                        | (monitorCoprocessor  ? MonitorCoprocessorBit  : 0)
                        | (emulation           ? EmulationBit           : 0)
                        | (taskSwitched        ? TaskSwitchedBit        : 0)
                        | (extensionType       ? ExtensionTypeBit       : 0)
                        | (numericError        ? NumericErrorBit        : 0)
                        | (writeProtect        ? WriteProtectBit        : 0)
                        | (alignmentMask       ? AlignmentMaskBit       : 0)
                        | (notWriteThrough     ? NotWriteThroughBit     : 0)
                        | (cacheDisable        ? CacheDisableBit        : 0)
                        | (paging              ? PagingBit              : 0);
        }

        /*  Properties  */

        BITPROPRW(ProtectedModeEnable)
        BITPROPRW(MonitorCoprocessor)
        BITPROPRW(Emulation)
        BITPROPRW(TaskSwitched)
        BITPROPRW(ExtensionType)
        BITPROPRW(NumericError)
        BITPROPRW(WriteProtect)
        BITPROPRW(AlignmentMask)
        BITPROPRW(NotWriteThrough)
        BITPROPRW(CacheDisable)
        BITPROPRW(Paging)

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

    /**
     * Represents the contents of the CR3 register.
     */
    struct Cr3
    {
        /*  Bit structure with PCID disabled:
         *       0 -   2 : Ignored
         *       3       : PWT (page-level write-through)
         *       4       : PCD (page-level cache disable)
         *       5 -  11 : Ignored
         *      12 - M-1 : Physical address of PML4 table; 4-KiB aligned.
         *       M -  63 : Reserved (must be 0)
         *
         *  Bit structure with PCID enabled:
         *       0 -  11 : PCID
         *      12 - M-1 : Physical address of PML4 table; 4-KiB aligned.
         *       M -  63 : Reserved (must be 0)
         */

        static const uint64_t PwtBit        = 1ULL <<  3;
        static const uint64_t PcdBit        = 1ULL <<  4;

        static const uint64_t AddressBits   = 0x000FFFFFFFFFF000ULL;
        static const uint64_t PcidBits      = 0x0000000000000FFFULL;

        /*  Constructors  */

        /**
         *  Creates a new CR3 structure from the given raw value.
         */
        __bland __forceinline Cr3(const uint64_t val)
        {
            this->Value = val;
        }

        /**
         *  Creates a new CR3 structure for use with PCID disabled.
         */
        __bland __forceinline Cr3(const paddr_t paddr, const bool PWT, const bool PCD)
        {
            this->Value = ((uint64_t)paddr & AddressBits)
                        | (PWT ? PwtBit : 0)
                        | (PCD ? PcdBit : 0);
        }

        /**
         *  Creates a new CR3 structure for use with PCID enabled.
         */
        __bland __forceinline Cr3(const paddr_t paddr, const Handle process)
        {
            //  TODO: Check whether the handle is correct or not.

            this->Value = ((uint64_t)paddr    & AddressBits)
                        | (process.GetIndex() & PcidBits   );
        }

        /*  Properties  */

        BITPROPRW(Pwt)
        BITPROPRW(Pcd)

        /**
         *  Gets the physical address of the PML4 table.
         */
        __bland __forceinline Pml4 * GetPml4Ptr() const
        {
            return (Pml4 *)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the PML4 table.
         */
        __bland __forceinline void SetPml4Ptr(const Pml4 * const paddr)
        {
            this->Value = ((uint64_t)paddr       &  AddressBits)
                        | (          this->Value & ~AddressBits);
        }

        /**
         *  Gets the physical address of the PML4 table.
         */
        __bland __forceinline paddr_t GetAddress() const
        {
            return (paddr_t)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the PML4 table.
         */
        __bland __forceinline void SetAddress(const paddr_t paddr)
        {
            this->Value = ((uint64_t)paddr       &  AddressBits)
                        | (          this->Value & ~AddressBits);
        }

        /*  PCID  */

        /**
         *  Gets the process ID according to the PCID value.
         */
        __bland __forceinline Handle GetProcess() const
        {
            //  TODO: Construct proper handles here!

            return Handle(HandleType::Process, this->Value & PcidBits);
        }
        /**
         *  Sets the PCID value from the given process.
         */
        __bland __forceinline void SetProcess(const Handle process)
        {
            //  TODO: Check whether the handle is correct or not.

            this->Value = (process.GetIndex() &  PcidBits)
                        | (       this->Value & ~PcidBits);
        }

        /**
         *  Gets the PCID value.
         */
        __bland __forceinline uint64_t GetPcid() const
        {
            //  TODO: Construct proper handles here!

            return this->Value & PcidBits;
        }
        /**
         *  Sets the PCID value.
         */
        __bland __forceinline void SetPcid(const uint64_t pcid)
        {
            //  TODO: Check whether the handle is correct or not.

            this->Value = (pcid        &  PcidBits)
                        | (this->Value & ~PcidBits);
        }

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

    //  TODO: CR4

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
