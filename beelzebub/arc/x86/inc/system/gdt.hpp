#pragma once

#include <utils/bitfields.hpp>

namespace Beelzebub { namespace System
{
    /**
     *  <summary>Represents an 8-byte entry in the GDT.</summary>
     */
    struct GdtEntryShort
    {
        /*  Bit structure with PCID disabled:
         *       0 -  15 : Segment Limit low
         *      16 -  39 : Segment Base low
         *      40       : Accessed
         *      41       : Readable code / Writable data
         *      42       : Conforming code / data Direction
         *      43       : Executable bit
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
        BITFIELD_DEFAULT_1W(47, Present    )
        BITFIELD_DEFAULT_1W(52, Available  )
        BITFIELD_DEFAULT_1W(53, Long       )
        BITFIELD_DEFAULT_1W(54, Size       )
        BITFIELD_DEFAULT_1W(55, Granularity)

        //BITFIELD_DEFAULT_2W(16, 24, uint32_t, BaseLow  )
        //BITFIELD_DEFAULT_2W(56,  8, uint32_t, BaseHigh )
        //BITFIELD_DEFAULT_2W( 0, 16, uint32_t, LimitLow )
        //BITFIELD_DEFAULT_2W(48,  4, uint32_t, LimitHigh)
        BITFIELD_DEFAULT_2W(45,  2,  uint8_t, Dpl)

        BITFIELD_DEFAULT_3W(16, 24, 56,  8, uint32_t, Base )
        BITFIELD_DEFAULT_3W( 0, 16, 48,  4, uint32_t, Limit)

        /*  Constructors  */

        /**
         *  <summary>Creates a null GDT Entry.</summary>
         */
        __bland inline GdtEntryShort()
            : Value(0)
        {
            
        }

        /**
         *  <summary>Creates a new GDT Entry structure from the given raw value.</summary>
         */
        __bland inline explicit GdtEntryShort(uint64_t const val)
            : Value(val)
        {
            
        }

        /*  Field(s)  */

        uint64_t Value;
    };

    class Gdt
    {
    public:

        /*  Constructor(s)  */

        Gdt() = default;

        Gdt(Gdt const &) = delete;
        Gdt & operator =(Gdt const &) = delete;

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

        __bland inline void Activate()
        {
            asm volatile ( "lgdt (%[ptr]) \n\t" : : [ptr]"r"(this) );
        }

        static __bland inline GdtRegister Retrieve()
        {
            GdtRegister res;

            asm volatile ( "sgdt %[ptr] \n\t" : : [ptr]"m"(res) );

            return res;
        }
    } __packed;
}}
