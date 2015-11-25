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

#include <system/tss.hpp>
#include <utils/bitfields.hpp>

namespace Beelzebub { namespace System
{
    enum class GdtSystemEntryType : uint8_t
    {
        Extension       =  0,
        Tss16Available  =  1,
        Ldt             =  2,
        Tss16Busy       =  3,
        CallGate16      =  4,
        TaskGate        =  5,
        InterruptGate16 =  6,
        TrapGate16      =  7,
        Reserved1       =  8,
        TssAvailable    =  9,
        Reserved2       = 10,
        TssBusy         = 11,
        CallGate        = 12,
        Reserved3       = 13,
        InterruptGate   = 14,
        TrapGate        = 15,
    };

    /**
     *  <summary>Represents an 8-byte entry in the GDT.</summary>
     */
    struct GdtEntryShort
    {
        /*  Bit structure:
         *       0 -  15 : Segment Limit low
         *      16 -  39 : Segment Base low
         *      40 -  43 : Type
         *          40   : Accessed
         *          41   : Readable code / Writable data
         *          42   : Conforming code / data Direction
         *          43   : Executable bit
         *      44       : System bit
         *      45 -  46 : Privilege bits
         *      47       : Present bit
         *      48 -  51 : Segment Limit high
         *      52       : Available bit
         *      53       : Long bit
         *      54       : Size bit
         *      55       : Granularity bit
         *      56 -  63 : Segment Base high
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1W(40, Accessed   )
        BITFIELD_DEFAULT_1W(41, Rw         )
        BITFIELD_DEFAULT_1W(42, Dc         )
        BITFIELD_DEFAULT_1W(43, Ex         )
        BITFIELD_DEFAULT_1W(44, System     )
        BITFIELD_DEFAULT_1W(45, DplLow     )
        BITFIELD_DEFAULT_1W(46, DplHigh    )
        BITFIELD_DEFAULT_1W(47, Present    )
        BITFIELD_DEFAULT_1W(52, Available  )
        BITFIELD_DEFAULT_1W(53, Long       )
        BITFIELD_DEFAULT_1W(54, Size       )
        BITFIELD_DEFAULT_1W(55, Granularity)

        BITFIELD_DEFAULT_4W(40,  4, GdtSystemEntryType, SystemDescriptorType)

        BITFIELD_DEFAULT_3W(16, 24, 56,  8, uint32_t, Base )
        BITFIELD_DEFAULT_3W( 0, 16, 48,  4, uint32_t, Limit)

        /*  Constructors  */

        /**
         *  <summary>Creates a null GDT Entry.</summary>
         */
        inline constexpr GdtEntryShort()
            : Value(0)
        {
            
        }

        /**
         *  <summary>Creates a new GDT Entry structure from the given raw value.</summary>
         */
        inline explicit constexpr GdtEntryShort(uint64_t const val)
            : Value(val)
        {
            
        }

        /**
         *  <summary>Creates a new GDT Entry structure from the given raw value.</summary>
         */
        inline constexpr GdtEntryShort(GdtSystemEntryType const type
                                             , uint32_t           const base = 0
                                             , uint32_t           const limit = 0
                                             , bool               const system = false)
            : Value((((uint64_t)type  << SystemDescriptorTypeOffset) & SystemDescriptorTypeBits)
                  | (((uint64_t)base  <<              BaseLowOffset) &              BaseLowBits)
                  | (((uint64_t)base  <<             BaseHighOffset) &             BaseHighBits)
                  | (((uint64_t)limit <<             LimitLowOffset) &             LimitLowBits)
                  | (((uint64_t)limit <<            LimitHighOffset) &            LimitHighBits)
                  | (system      ?      SystemBit : 0ULL))
        {
            
        }

        /*  Field(s)  */

        uint64_t Value;
    };

    /**
     *  <summary>Represents a 16-byte TSS entry in the GDT.</summary>
     */
    struct GdtTss64Entry
    {
        /*  Bit structure:
         *       0 -  15 : Segment Limit low
         *      16 -  39 : Base Address low
         *      40 -  43 : Type
         *          40   : 1
         *          41   : Busy
         *          42   : 0
         *          43   : 1
         *      44       : 0
         *      45 -  46 : Privilege bits
         *      47       : Present bit
         *      48 -  51 : Segment Limit high
         *      52       : Available bit
         *      53       : 0
         *      54       : 0
         *      55       : Granularity bit
         *      56 -  95 : Segment Base high
         *      96 - 103 : Reserved
         *     104 - 108 : Zeros
         *     109 - 127 : Reserved
         */

        BITFIELD_FLAG_RW(41, Busy       , uint64_t, this->Low, , const, static)
        BITFIELD_FLAG_RW(45, DplLow     , uint64_t, this->Low, , const, static)
        BITFIELD_FLAG_RW(46, DplHigh    , uint64_t, this->Low, , const, static)
        BITFIELD_FLAG_RW(47, Present    , uint64_t, this->Low, , const, static)
        BITFIELD_FLAG_RW(52, Available  , uint64_t, this->Low, , const, static)
        BITFIELD_FLAG_RW(55, Granularity, uint64_t, this->Low, , const, static)

        BITFIELD_STRO_RW(40,  4, GdtSystemEntryType, SystemDescriptorType, uint64_t, this->Low, , const, static)

        BITFIELD_STRC_RW(16, 24, 56,  8, uint32_t, BaseLow, uint64_t, this->Low, , const, static)
        BITFIELD_STRC_RW( 0, 16, 48,  4, uint32_t, Limit  , uint64_t, this->Low, , const, static)

        /*  Constructors  */

        /**
         *  <summary>Creates a null GDT Entry.</summary>
         */
        inline constexpr GdtTss64Entry()
            : Low(0x0000000000900000), High(0)
        {
            
        }

        /**
         *  <summary>Creates a new GDT Entry structure from the given raw value.</summary>
         */
        inline constexpr GdtTss64Entry(uint64_t const low, uint64_t const high)
            : Low(low), High(high)
        {
            
        }

        /*  Properties  */

        inline Tss * GetBase() const
        {
            return reinterpret_cast<Tss *>((uint64_t)this->GetBaseLow()
                                        | ((uint64_t)this->BaseHigh << 32));
        }
        inline auto SetBase(Tss const * const val)
        {
            this->SetBaseLow((uint32_t) reinterpret_cast<uint64_t>(val)      );
            this->BaseHigh = (uint32_t)(reinterpret_cast<uint64_t>(val) >> 32);

            return *this;
        }

        /*  Field(s)  */

        uint64_t Low;

        union
        {
            uint64_t High;

            struct
            {
                uint32_t BaseHigh;
                uint32_t Dummy0;
            };
        };
    };

    class Gdt
    {
    public:

        /*  Statics  */

        static uint16_t const KernelCodeSegment = 0x08;
        static uint16_t const KernelDataSegment = 0x10;
        static uint16_t const UserCodeSegment = 0x18;   //  TODO: implement properly.

        /*  Constructor(s)  */

        Gdt() = default;

        Gdt(Gdt const &) = delete;
        Gdt & operator =(Gdt const &) = delete;

        /*  Getters  */

        inline GdtTss64Entry & GetTss64(uint16_t const seg)
        {
            return *(reinterpret_cast<GdtTss64Entry *>(
                reinterpret_cast<uint8_t *>(this->Entries) + seg
            ));
        }

        /*  Field(s)  */

        GdtEntryShort Entries[1];
    };

    /**
     *  <summary>Structure of the GDTR.</summary>
     */
    struct GdtRegister
    {
        /*  Field(s)  */

        uint16_t Size;
        Gdt * Pointer;

        /*  Load & Store  */

        inline void Activate()
        {
            asm volatile ( "lgdt (%[ptr]) \n\t" : : [ptr]"r"(this) );
        }

        static inline GdtRegister Retrieve()
        {
            GdtRegister res;

            asm volatile ( "sgdt %[ptr] \n\t" : : [ptr]"m"(res) );

            return res;
        }
    } __packed;
}}
