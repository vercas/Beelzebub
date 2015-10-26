#pragma once

#include <system/registers_x86.hpp>
#include <memory/paging.hpp>
#include <handles.h>

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

        static uint64_t const ProtectedModeEnableBit = 1ULL <<  0;
        static uint64_t const MonitorCoprocessorBit  = 1ULL <<  1;
        static uint64_t const EmulationBit           = 1ULL <<  2;
        static uint64_t const TaskSwitchedBit        = 1ULL <<  3;
        static uint64_t const ExtensionTypeBit       = 1ULL <<  4;
        static uint64_t const NumericErrorBit        = 1ULL <<  5;
        static uint64_t const WriteProtectBit        = 1ULL << 16;
        static uint64_t const AlignmentMaskBit       = 1ULL << 18;
        static uint64_t const NotWriteThroughBit     = 1ULL << 29;
        static uint64_t const CacheDisableBit        = 1ULL << 30;
        static uint64_t const PagingBit              = 1ULL << 31;

        static uint64_t const ProtectedModeEnableBitIndex    =  0;
        static uint64_t const MonitorCoprocessorBitIndex     =  1;
        static uint64_t const EmulationBitIndex              =  2;
        static uint64_t const TaskSwitchedBitIndex           =  3;
        static uint64_t const ExtensionTypeBitIndex          =  4;
        static uint64_t const NumericErrorBitIndex           =  5;
        static uint64_t const WriteProtectBitIndex           = 16;
        static uint64_t const AlignmentMaskBitIndex          = 18;
        static uint64_t const NotWriteThroughBitIndex        = 29;
        static uint64_t const CacheDisableBitIndex           = 30;
        static uint64_t const PagingBitIndex                 = 31;

        /*  Constructors  */

        /**
         *  Creates a new CR0 structure from the given raw value.
         */
        __bland __forceinline Cr0(uint64_t const val)
        {
            this->Value = val;
        }

        /**
         *  Creates a new CR0 structure with the given flags.
         */
        __bland __forceinline Cr0(bool const protectedModeEnable
                                , bool const monitorCoprocessor
                                , bool const emulation
                                , bool const taskSwitched
                                , bool const extensionType
                                , bool const numericError
                                , bool const writeProtect
                                , bool const alignmentMask
                                , bool const notWriteThrough
                                , bool const cacheDisable
                                , bool const paging)
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

        BITPROPRW(ProtectedModeEnable, Value)
        BITPROPRW(MonitorCoprocessor, Value)
        BITPROPRW(Emulation, Value)
        BITPROPRW(TaskSwitched, Value)
        BITPROPRW(ExtensionType, Value)
        BITPROPRW(NumericError, Value)
        BITPROPRW(WriteProtect, Value)
        BITPROPRW(AlignmentMask, Value)
        BITPROPRW(NotWriteThrough, Value)
        BITPROPRW(CacheDisable, Value)
        BITPROPRW(Paging, Value)

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

        static uint64_t const PwtBit        = 1ULL <<  3;
        static uint64_t const PcdBit        = 1ULL <<  4;

        static uint64_t const PwtBitIndex           =  3;
        static uint64_t const PcdBitIndex           =  4;

        static uint64_t const AddressBits   = 0x000FFFFFFFFFF000ULL;
        static uint64_t const PcidBits      = 0x0000000000000FFFULL;

        /*  Constructors  */

        /**
         *  Creates a new CR3 structure from the given raw value.
         */
        __bland inline Cr3(uint64_t const val)
            : Value(val)
        {
            
        }

        /**
         *  Creates a new CR3 structure for use with PCID disabled.
         */
        __bland __forceinline Cr3(paddr_t const paddr, bool const PWT, bool const PCD)
            : Value(((uint64_t)paddr & AddressBits)
                  | (PWT ? PwtBit : 0)
                  | (PCD ? PcdBit : 0))
        {
            
        }

        /**
         *  Creates a new CR3 structure for use with PCID enabled.
         */
        __bland __forceinline Cr3(paddr_t const paddr, Handle const process)
            : Value(((uint64_t)paddr    & AddressBits)
                  | (process.GetIndex() & PcidBits   ))
        {
            //  TODO: Check whether the handle is correct or not.
        }

        /*  Properties  */

        BITPROPRW(Pwt, Value)
        BITPROPRW(Pcd, Value)

        /**
         *  Gets the physical address of the PML4 table.
         */
        __bland __forceinline Memory::Pml4 * GetPml4Ptr() const
        {
            return (Memory::Pml4 *)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the PML4 table.
         */
        __bland __forceinline void SetPml4Ptr(Memory::Pml4 const * const paddr)
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
        __bland __forceinline void SetAddress(paddr_t const paddr)
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
        __bland __forceinline void SetProcess(Handle const process)
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
        __bland __forceinline void SetPcid(uint64_t const pcid)
        {
            //  TODO: Check whether the handle is correct or not.

            this->Value = (pcid        &  PcidBits)
                        | (this->Value & ~PcidBits);
        }

        /*  Field(s)  */

    //private:

        uint64_t Value;
    };

    //  TODO: CR4
}}
