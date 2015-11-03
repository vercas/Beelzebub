#pragma once

#include <utils/bitfields.hpp>

namespace Beelzebub { namespace System
{
    /**
     *  <summary>Known LAPIC.</summary>
     */
    enum class LapicRegister
        : uint16_t
    {
        LapicId                      = 0x0002,
        SpuriousInterruptVector      = 0x000F,
        EndOfInterrupt               = 0x000B,
        InterruptCommandRegisterLow  = 0x0030,
        InterruptCommandRegisterHigh = 0x0031,
        Timer                        = 0x0032,
        TimerInitialCount            = 0x0038,
        TimerCurrentCount            = 0x0039,
        TimerDivisor                 = 0x003E,
    };

    /**
     *  <summary>
     *  Represents the contents of the Spurious-Interrupt Vector Register of the
     *  LAPIC.
     *  </summary>
     */
    struct LapicSvr
    {
        /*  Bit structure:
         *       0 -   7 : Spurious Vector
         *       8       : APIC Software Enable
         *       9       : Focus Processor Checking
         *      10 -  11 : Reserved (must be 0)
         *      12       : EOI Broadcast Suppression
         *      13 -  31 : Reserved (must be 0)
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1O( 8, BootstrapProcessor)
        BITFIELD_DEFAULT_1W(10, X2ApicEnabled     )
        BITFIELD_DEFAULT_1W(11, GlobalLapicEnabled)

        BITFIELD_DEFAULT_2W(12,  24, paddr_t, ApicBase)

        /*  Constructor  */

        /**
         *  Creates a new LAPIC SVR structure from the given raw value.
         */
        __bland inline explicit LapicSvr(uint32_t const val)
        {
            this->Value = val;
        }

        /*  Field(s)  */

    //private:

        uint32_t Value;
    };
}}
