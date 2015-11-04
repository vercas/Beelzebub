#pragma once

#include <system/interrupts.hpp>
#include <utils/bitfields.hpp>

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
         *  Creates a new PIT command register structure from the given raw value.
         */
        __bland inline explicit constexpr PitCommand(uint8_t const val)
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
    struct DividerFrequency
    {
        uint16_t Divider;
        uint32_t Frequency;
    };

    /**
     *  <summary>Contains methods for interacting with the PIT.</summary>
     */
    class Pit
    {
    public:
        /*  Statics  */

        static uint32_t const BaseFrequency = 1193182;

        /*  IRQ Handler  */

        static __bland void IrqHandler(IsrState * const state, InterruptEnderFunction const ender);

        /*  Constructor(s)  */

    protected:
        Pit() = default;

    public:
        Pit(Pit const &) = delete;
        Pit & operator =(Pit const &) = delete;

    public:
        /*  Initialization  */

        static __cold __bland void SetFrequency(uint32_t & freq);

        static __cold __bland void SendCommand(PitCommand const cmd);

        /*  Utilities  */

        static __bland inline DividerFrequency GetRealFrequency(uint32_t freq)
        {
            uint32_t ret;

            asm ( "xorl %%edx, %%edx \n\t"  //  Set D to 0
                  "movl %%ecx, %%eax \n\t"  //  Set A to base frequency
                  "divl %%ebx        \n\t"  //  Divide base frequency by freq
                  "movl %%eax, %%ebx \n\t"  //  Set B to result (divider)
                  "xorl %%edx, %%edx \n\t"  //  Set D to 0
                  "movl %%ecx, %%eax \n\t"  //  Set A to base frequency
                  "divl %%ebx        \n\t"  //  Divide base frequency by divider
                : "=a"(ret), "+b"(freq)
                : "c"(BaseFrequency)
                : "edx");
            //  My choice of registers here is due to the fact that I cannot
            //  trust the compiler not to do funny allocations.

            //  Yes, `freq` becomes the divider after the previous block.
            return {(uint16_t)freq, ret};
        }
    };
}}}
