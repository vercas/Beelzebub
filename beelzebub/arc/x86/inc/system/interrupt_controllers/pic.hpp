#pragma once

#include <system/interrupts.hpp>
#include <utils/bitfields.hpp>

namespace Beelzebub { namespace System { namespace InterruptControllers
{
    /**
     *  <summary>Contains methods for interacting with the PIC.</summary>
     */
    class Pic
    {
    public:
        /*  Statics  */

        static uint16_t const MasterCommandPort = 0x20;
        static uint16_t const MasterDataPort = 0x21;
        static uint16_t const SlaveCommandPort = 0xA0;
        static uint16_t const SlaveDataPort = 0xA1;

        static uint8_t VectorOffset;

        /*  Ender  */

        static __hot __bland void IrqEnder(INTERRUPT_ENDER_ARGS);

        /*  Constructor(s)  */

    protected:
        Pic() = default;

    public:
        Pic(Pic const &) = delete;
        Pic & operator =(Pic const &) = delete;

    public:
        /*  (De)initialization  */

        static __cold __bland void Initialize(uint8_t const vecOff);
        static __cold __bland void Disable();

        /*  Subscription  */

        static __bland bool Subscribe(uint8_t const irq, InterruptHandlerFunction const handler);
        static __bland bool Unsubscribe(uint8_t const irq);
    };
}}}
