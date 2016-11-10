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

#include <system/interrupts.hpp>
#include <utils/bitfields.hpp>
#include <synchronization/atomic.hpp>

namespace Beelzebub { namespace System { namespace Timers
{
    /**
     *  <summary>
     *  Known PIT channels for the command register.
     *  </summary>
     */
    enum class PitChannel : uint8_t
    {
        Channel0 = 0,
        Channel1 = 1,
        Channel2 = 2,
        ReadBack = 3,
    };

    /**
     *  <summary>
     *  Known access modes for the PIT command register.
     *  </summary>
     */
    enum class PitAccessMode : uint8_t
    {
        LatchCountValue = 0,
        LowByte         = 1,
        HighByte        = 2,
        LowHigh         = 3,
    };

    /**
     *  <summary>
     *  Known operating modes for the PIT command register.
     *  </summary>
     */
    enum class PitOperatingMode : uint8_t
    {
        InterruptOnTerminalCount = 0,
        HardwareRetriggerableOneShot = 1,
        RateGenerator = 2,
        SquareWaveGenerator = 3,
        SoftwareTriggeredStrobe = 4,
        HardwareTriggeredStrobe = 5,
        RateGeneratorEx = 6,
        SquareWaveGeneratorEx = 7,
    };

    /**
     *  <summary>
     *  Represents the contents of the command register of the PIT.
     *  </summary>
     */
    struct PitCommand
    {
        /*  Bit structure:
         *       0       : BCD mode
         *       1 -   3 : Operating Mode
         *       4 -   5 : Access Mode
         *       6 -   7 : Channel Selection
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1OEx( 0                      , BcdMode      , 8)
        BITFIELD_DEFAULT_4WEx( 1,  3, PitOperatingMode, OperatingMode, 8)
        BITFIELD_DEFAULT_4WEx( 4,  2, PitAccessMode   , AccessMode   , 8)
        BITFIELD_DEFAULT_4WEx( 6,  2, PitChannel      , Channel      , 8)

        /*  Constructor  */

        /**
         *  <summary>Creates a null PIT command register structure.</summary>
         */
        inline constexpr PitCommand() : Value(0) { }

        /**
         *  <summary>
         *  Creates a new PIT command register structure from the given raw
         *  value.
         *  </summary>
         */
        inline explicit constexpr PitCommand(uint8_t const val)
            : Value(val)
        {
            
        }

        /*  Field(s)  */

    //private:

        uint8_t Value;
    };

    /**
     *  <summary>Contains methods for interacting with the PIT.</summary>
     */
    class Pit
    {
    public:
        /*  Statics  */

        static uint32_t const BaseFrequency = 1193182;
        static uint32_t const MinimumFrequency = 19;

        static Synchronization::Atomic<size_t> Counter;

        static uint32_t Period, Frequency; //  In microseconds.

        /*  IRQ Handler  */

        static uint8_t const IrqNumber = 0;

        static void IrqHandler(INTERRUPT_HANDLER_ARGS_FULL);

        /*  Constructor(s)  */

    protected:
        Pit() = default;

    public:
        Pit(Pit const &) = delete;
        Pit & operator =(Pit const &) = delete;

        /*  Initialization  */

        static __cold void SetFrequency(uint32_t freq);
        static __cold void SetHandler(InterruptHandlerFullFunction han = nullptr);

        static __cold void SendCommand(PitCommand const cmd);
    };
}}}
