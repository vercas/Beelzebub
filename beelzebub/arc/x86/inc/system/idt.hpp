#pragma once

#include <utils/bitfields.hpp>

namespace Beelzebub { namespace System
{
    enum class IdtGateType : uint8_t
    {
        Unused00        =  0,
        Unused01        =  1,
        Unused02        =  2,
        Unused03        =  3,
        Unused04        =  4,
        TaskGate        =  5,
        InterruptGate16 =  6,
        TrapGate16      =  7,
        Unused08        =  8,
        Unused09        =  9,
        Unused10        = 10,
        Unused11        = 11,
        Unused12        = 12,
        Unused13        = 13,
        InterruptGate   = 14,
        TrapGate        = 15,
    };

#include <system/idt_gate.inc>

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

        __bland inline void Activate()
        {
            asm volatile ( "lidt (%[ptr]) \n\t" : : [ptr]"r"(this) );
        }

        static __bland inline IdtRegister Retrieve()
        {
            IdtRegister res;

            asm volatile ( "sidt %[ptr] \n\t" : : [ptr]"m"(res) );

            return res;
        }
    } __packed;
}}
