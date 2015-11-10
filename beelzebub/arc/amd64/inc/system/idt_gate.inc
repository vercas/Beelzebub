#pragma once

/**
 *  <summary>Represents a 16-byte entry in the IDT.</summary>
 */
union IdtGate
{
    /*  This is implemented as a union for layout optimization...
     *  x86 supports unaligned access, which is used to cheat the Offset.
     *
     *  Bit structure:
     *       0 -  15 : Offset low
     *      16 -  31 : Segment Selector
     *      32 -  34 : Interrupt Stack Table
     *      35       : Zero
     *      36       : Zero
     *      37 -  39 : Zeros
     *      40 -  43 : Type
     *      44       : Zero
     *      45 -  46 : DPL
     *      47       : Present
     *      48 -  63 : Offset middle
     *      64 -  95 : Offset high
     *      96 - 127 : Reserved
     */

    /*  Properties  */

    BITFIELD_FLAG_RW(45, DplLow , uint64_t, this->Low, __bland, const, static)
    BITFIELD_FLAG_RW(46, DplHigh, uint64_t, this->Low, __bland, const, static)
    BITFIELD_FLAG_RW(47, Present, uint64_t, this->Low, __bland, const, static)

    BITFIELD_STRO_RW(32,  3, uint8_t    , Ist , uint64_t, this->Low, __bland, const, static)
    BITFIELD_STRO_RW(40,  4, IdtGateType, Type, uint64_t, this->Low, __bland, const, static)

    /*  Constructors  */

    /**
     *  <summary>Creates a null IDT Entry.</summary>
     */
    __bland inline constexpr IdtGate()
        : Low(0), High(0)
    {
        
    }

    /**
     *  <summary>Creates a new IDT Entry structure from the given raw values.</summary>
     */
    __bland inline explicit constexpr IdtGate(uint64_t const low, uint64_t const high)
        : Low(low), High(high)
    {
        
    }

    /*  Properties  */

    __bland inline uintptr_t GetOffset() const
    {
        return (uintptr_t)this->OffsetLow | (this->OffsetHigh & 0xFFFFFFFFFFFF0000);
    }

    __bland inline auto SetOffset(const uintptr_t val)
    {
        this->OffsetLow  =  val              & 0x000000000000FFFF;
        this->OffsetHigh = (val              & 0xFFFFFFFFFFFF0000)
                         | (this->OffsetHigh & 0x000000000000FFFF);

        //  Simple and elegant.

        return *this;
    }

    __bland inline uint16_t GetSegment() const
    {
        return this->Segment;
    }

    __bland inline auto SetSegment(const uint16_t val)
    {
        this->Segment = val;

        return *this;
    }

    /*  Field(s)  */

    struct
    {
        uint64_t Low;
        uint64_t High;
    } __packed;

    struct
    {
        uint16_t OffsetLow;
        uint16_t Segment;
        uint64_t OffsetHigh;    //  The lower word will be discarded.
        uint32_t Reserved0;
    } __packed;
};
