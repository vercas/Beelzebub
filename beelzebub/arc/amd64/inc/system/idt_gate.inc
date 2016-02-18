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

    BITFIELD_FLAG_RW(45, DplLow , uint64_t, this->Low, , const, static)
    BITFIELD_FLAG_RW(46, DplHigh, uint64_t, this->Low, , const, static)
    BITFIELD_FLAG_RW(47, Present, uint64_t, this->Low, , const, static)

    BITFIELD_STRO_RW(32,  3, uint8_t    , Ist , uint64_t, this->Low, , const, static)
    BITFIELD_STRO_RW(40,  4, IdtGateType, Type, uint64_t, this->Low, , const, static)

    /*  Constructors  */

    /**
     *  <summary>Creates a null IDT Entry.</summary>
     */
    inline constexpr IdtGate()
        : Low(0), High(0)
    {
        
    }

    /**
     *  <summary>Creates a new IDT Entry structure from the given raw values.</summary>
     */
    inline explicit constexpr IdtGate(uint64_t const low, uint64_t const high)
        : Low(low), High(high)
    {
        
    }

    /*  Properties  */

    inline uintptr_t GetOffset() const
    {
        return (uintptr_t)this->OffsetLow | (this->OffsetHigh & 0xFFFFFFFFFFFF0000);
    }

    inline auto SetOffset(const uintptr_t val)
    {
        this->OffsetLow  =  val              & 0x000000000000FFFF;
        this->OffsetHigh = (val              & 0xFFFFFFFFFFFF0000)
                         | (this->OffsetHigh & 0x000000000000FFFF);

        //  Simple and elegant.

        return *this;
    }

    inline uint16_t GetSegment() const
    {
        return this->Segment;
    }

    inline auto SetSegment(const uint16_t val)
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
