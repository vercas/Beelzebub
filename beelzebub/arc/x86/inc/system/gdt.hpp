#pragma once

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

        BITFIELD_DEFAULT_2W( 0, 32, uintptr_t         , ExtendedTssBaseHigh )
        BITFIELD_DEFAULT_4W(40,  4, GdtSystemEntryType, SystemDescriptorType)

        BITFIELD_DEFAULT_3W(16, 24, 56,  8, uint32_t, Base )
        BITFIELD_DEFAULT_3W( 0, 16, 48,  4, uint32_t, Limit)

        /*  Constructors  */

        /**
         *  <summary>Creates a null GDT Entry.</summary>
         */
        __bland inline constexpr GdtEntryShort()
            : Value(0)
        {
            
        }

        /**
         *  <summary>Creates a new GDT Entry structure from the given raw value.</summary>
         */
        __bland inline explicit constexpr GdtEntryShort(uint64_t const val)
            : Value(val)
        {
            
        }

        /**
         *  <summary>Creates a new GDT Entry structure from the given raw value.</summary>
         */
        __bland inline constexpr GdtEntryShort(GdtSystemEntryType const type
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
        /*  Constructors  */

        /**
         *  <summary>Creates a null GDT Entry.</summary>
         */
        __bland inline constexpr GdtTss64Entry()
            : LowEntry(), HighEntry()
        {
            
        }

        /**
         *  <summary>Creates a new GDT Entry structure from the given raw value.</summary>
         */
        __bland inline constexpr GdtTss64Entry(GdtEntryShort const low, GdtEntryShort const high)
            : LowEntry(low), HighEntry(high)
        {
            
        }

        /*  Properties  */

        __bland inline bool GetBusy() const
        {
            return this->LowEntry.GetRw();
        }
        __bland inline void SetBusy(bool const val)
        {
            this->LowEntry.SetRw(val);
        }

        /*  Field(s)  */

        GdtEntryShort  LowEntry;
        GdtEntryShort HighEntry;
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
