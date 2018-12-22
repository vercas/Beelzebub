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

/*
    Interrupt vectors assignment in Beelzebub on x86:
      0 -  31 : CPU exceptions
     32 - 239 : I/O
    240 - 255 : Internal Use
        254       : APIC Timer
        255       : IPI
 */

#pragma once

#include "utils/bitfields.hpp"
#include <beel/structs.kernel.h>

namespace Beelzebub { namespace System
{
    /**
     *  The stub entered by each syscall.
     */
    union IsrStub
    {
        /*  Static(s)  */

        static constexpr uint8_t PushRcxValue = 0x51;

        /*  Properties  */

        inline bool ProvidesErrorCode()
        {
            return this->Bytes[0] == PushRcxValue;
        }

        inline void * GetJumpBase()
        {
            if (this->ProvidesErrorCode())
                return &(this->WithErrorCode.JumpOffset) + 1;
            else
                return &(this->NoErrorCode.JumpOffset) + 1;
        }

        inline void * GetJumpTarget()
        {
            if (this->ProvidesErrorCode())
                return reinterpret_cast<uint8_t *>(&(this->WithErrorCode.JumpOffset) + 1) + this->WithErrorCode.JumpOffset;
            else
                return reinterpret_cast<uint8_t *>(&(this->NoErrorCode.JumpOffset) + 1) + this->NoErrorCode.JumpOffset;
        }

        inline IsrStub & SetJumpTarget(void const * const val)
        {
            if (this->ProvidesErrorCode())
                this->WithErrorCode.JumpOffset =
                    (int32_t)reinterpret_cast<int64_t>(
                        reinterpret_cast<intptr_t>(val)
                        - reinterpret_cast<intptr_t>(&(this->WithErrorCode.JumpOffset) + 1));
            else
                this->NoErrorCode.JumpOffset =
                    (int32_t)reinterpret_cast<int64_t>(
                        reinterpret_cast<intptr_t>(val)
                        - reinterpret_cast<intptr_t>(&(this->NoErrorCode.JumpOffset) + 1));

            return *this;
        }

        /*  Field(s)  */

        uint8_t Bytes[16];

        struct
        {
            uint8_t PushRcx;
            uint8_t MovCl;
            uint32_t Vector;
            uint8_t Jump;
            int32_t JumpOffset;
        } __packed WithErrorCode;

        struct
        {
            uint8_t PushImmediate;
            uint8_t DummyErrorCode;
            uint8_t PushRcx;
            uint8_t MovCl;
            uint32_t Vector;
            uint8_t Jump;
            int32_t JumpOffset;
        } __packed NoErrorCode;
    };

    static_assert(sizeof(IsrStub) == 16, "Struct size mismatch.");

    __extern IsrStub IsrStubsBegin, IsrStubsEnd;

    /**
     *  IDT gate types
     */
    #define ENUM_IDTGATETYPE(ENUMINST) \
        ENUMINST(Unused00       ,  0) \
        ENUMINST(Unused01       ,  1) \
        ENUMINST(Unused02       ,  2) \
        ENUMINST(Unused03       ,  3) \
        ENUMINST(Unused04       ,  4) \
        ENUMINST(TaskGate       ,  5) \
        ENUMINST(InterruptGate16,  6) \
        ENUMINST(TrapGate16     ,  7) \
        ENUMINST(Unused08       ,  8) \
        ENUMINST(Unused09       ,  9) \
        ENUMINST(Unused10       , 10) \
        ENUMINST(Unused11       , 11) \
        ENUMINST(Unused12       , 12) \
        ENUMINST(Unused13       , 13) \
        ENUMINST(InterruptGate  , 14) \
        ENUMINST(TrapGate       , 15)

    __ENUMDECL(IdtGateType, ENUM_IDTGATETYPE, LITE, uint8_t)
    __ENUM_TO_STRING_DECL(IdtGateType)

#ifdef __BEELZEBUB__ARCH_AMD64
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

        inline auto SetOffset(const uintptr_t val) -> decltype(*this)
        {
            this->OffsetLow  =  val              & 0x000000000000FFFF;
            this->OffsetHigh = (val              & 0xFFFFFFFFFFFF0000)
                             | (this->OffsetHigh & 0x000000000000FFFF);

            //  Simple and elegant.

            return *this;
        }

        inline auto SetOffset(const IsrStub * const val) -> decltype(*this)
        {
            return this->SetOffset(reinterpret_cast<uintptr_t>(val));
        }

        inline uint16_t GetSegment() const
        {
            return this->Segment;
        }

        inline auto SetSegment(const uint16_t val) -> decltype(*this)
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
#endif

    class Idt
    {
    public:

        /*  Constructor(s)  */

        Idt() = default;

        Idt(Idt const &) = delete;
        Idt & operator =(Idt const &) = delete;

        /*  Field(s)  */

        IdtGate Entries[256];
    };

    /**
     *  <summary>Structure of the GDTR.</summary>
     */
    struct IdtRegister
    {
        /*  Field(s)  */

        uint16_t Size;
        Idt * Pointer;

        /*  Load & Store  */

        inline void Activate()
        {
            asm volatile ( "lidt (%[ptr]) \n\t" : : [ptr]"r"(this) );
        }

        static inline IdtRegister Retrieve()
        {
            IdtRegister res;

            asm volatile ( "sidt %[ptr] \n\t" : : [ptr]"m"(res) );

            return res;
        }
    } __packed;

    constexpr uint8_t const IrqsOffset = 32;

    #define ENUM_KNOWNISRS(E) \
        E(DivideError                , 0  ) \
        E(Debug                      , 1  ) \
        E(Nmi                        , 2  ) \
        E(Breakpoint                 , 3  ) \
        E(Overflow                   , 4  ) \
        E(BoundRangeExceeded         , 5  ) \
        E(InvalidOpcode              , 6  ) \
        E(NoMathCoprocessor          , 7  ) \
        E(DoubleFault                , 8  ) \
        E(CoprocessorSegmentOverrun  , 9  ) \
        E(InvalidTss                 , 10 ) \
        E(SegmentNotPresent          , 11 ) \
        E(StackSegmentFault          , 12 ) \
        E(GeneralProtectionFault     , 13 ) \
        E(PageFault                  , 14 ) \
        E(Reserved1                  , 15 ) \
        E(FloatingPointError         , 16 ) \
        E(AlignmentCheck             , 17 ) \
        E(MachineCheck               , 18 ) \
        E(SimdFloatingPointException , 19 ) \
        E(ApicTimer                  , 254) \
        E(Mailbox                    , 255)

    __ENUMDECL(KnownIsrs, ENUM_KNOWNISRS, LITE, uint8_t)

    /************************
        Interrupt Vectors
    ************************/

    typedef GeneralRegisters64 InterruptStackState;

    #define INTERRUPT_ENDER_ARGS                           \
          void const * const handler                       \
        , uint8_t const vector

    typedef void (*InterruptEnderFunction)(INTERRUPT_ENDER_ARGS);

    #define INTERRUPT_HANDLER_ARGS                              \
          Beelzebub::System::InterruptStackState * const state  \
        , void const * const handler                            \
        , uint8_t const vector

    typedef void (*InterruptHandlerFunction)(INTERRUPT_HANDLER_ARGS);

    /**
     *  Represents the interrupt state of the system
     */
    class Interrupts
    {
    public:
        /*  Statics  */

        static constexpr size_t const Count = 256;
        static constexpr size_t const StubSize = 16;

        static Idt Table;
        static IdtRegister Register;

        /*  Subtypes  */

        class Data
        {
            /*  Field(s)  */

            uint8_t Vector;

        public:
            /*  Constructor(s)  */

            inline constexpr Data(uint8_t const vec) : Vector(vec) { }

            /*  Handler & Ender  */

            InterruptHandlerFunction GetHandler() const;

            Data const & SetHandler(InterruptHandlerFunction const val) const;
            Data const & RemoveHandler() const;

            /*  Properties  */

            inline uint8_t GetVector() const
            {
                return this->Vector;
            }

            /*  Gate & Stub  */

            inline IsrStub * GetStub() const
            {
                return &IsrStubsBegin + this->Vector;
            }

            inline IdtGate * GetGate() const
            {
                return Table.Entries + this->Vector;
            }

            inline Data const & SetGate(IdtGate const val) const
            {
                Table.Entries[this->Vector] = val;

                return *this;
            }
        };

        /*  Constructor(s)  */

    protected:
        Interrupts() = default;

    public:
        Interrupts(Interrupts const &) = delete;
        Interrupts & operator =(Interrupts const &) = delete;

        /*  Triggering  */

        template<uint8_t iVec>
        static __forceinline void Trigger()
        {
            asm volatile("int %0 \n\t"
                        : : "i"(iVec));
        }

        /*  Data  */

        static Data Get(uint8_t const vec)
        {
            return { vec };
        }

        static Data Get(KnownIsrs const vec)
        {
            return Get((uint8_t)vec);
        }

        /*  Status  */

        static inline void Enable()
        {
            asm volatile("sti \n\t" : : : "memory");
            //  This is a memory barrier to prevent the compiler from moving things around it.
        }

        static inline void Disable()
        {
            asm volatile("cli \n\t" : : : "memory");
            //  This is a memory barrier to prevent the compiler from moving things around it.
        }
    };
}}
