#pragma once

#include <system/isr.hpp>
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

        /*  Constructor(s)  */

    protected:
        Pic() = default;

    public:
        Pic(Pic const &) = delete;
        Pic & operator =(Pic const &) = delete;

    public:
        /*  Initialization  */

        static __cold __bland void Initialize();
    };
}}}
