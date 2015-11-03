#pragma once

#include <system/io_ports.hpp>
#include <system/isr.hpp>

namespace Beelzebub { namespace System { namespace Timers
{
	/**
     *  <summary>Contains methods for interacting with the PIT.</summary>
     */
    class Pit
    {
        /*  Constructor(s)  */

    protected:
        Pit() = default;

    public:
        Pit(Pit const &) = delete;
        Pit & operator =(Pit const &) = delete;

    public:
        /*  Initialization  */

        static __cold __bland void Initialize();
    };
}}}
