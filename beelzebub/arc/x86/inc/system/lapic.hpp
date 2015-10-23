#pragma once

#include <metaprogramming.h>

namespace Beelzebub { namespace System {
    /**
     *  Contains methods for interacting with the local APIC.
     */
    enum class LapicRegister
        : uint16_t
    {
        LapicId                      = 0x0002,
        EndOfInterrupt               = 0x000B,
        InterruptCommandRegisterLow  = 0x0030,
        InterruptCommandRegisterHigh = 0x0031,
        Timer                        = 0x0032,
        TimerInitialCount            = 0x0038,
        TimerCurrentCount            = 0x0039,
        TimerDivisor                 = 0x003E,
    };

    /**
     *  Contains methods for interacting with the local APIC.
     */
    class Lapic
    {
    public:
        /*  Initialization  */

        static __cold __bland void Initialize();

        /*  Registers  */

        static __hot __bland uint32_t ReadRegister(LapicRegister const reg);
        static __hot __bland void WriteRegister(LapicRegister const reg, uint32_t const value);
        //  These are normally the type of functions I inline forcefully...
        //  But I'll let the compiler decide for me this time. It seems to
        //  be more aggressive than I am anyway.

        /*  Shortcuts  */

        static __hot __bland __forceinline uint32_t GetId()
        {
            return ReadRegister(LapicRegister::LapicId);
        }

        static __hot __bland __forceinline void EndOfInterrupt()
        {
            WriteRegister(LapicRegister::EndOfInterrupt, 0);
        }
    };
}}
