#pragma once

#include <system/isr.hpp>

namespace Beelzebub { namespace Execution
{
    /**
     *  Represents the execution state of a thread.
     */
    /*struct ThreadState
    {
        uint64_t RBX;
        uint64_t RCX;
        uint64_t R12;
        uint64_t R13;
        uint64_t R14;
        uint64_t R15;
        uint64_t RBP;
        uint64_t RIP;
    };//*/

    typedef Beelzebub::System::IsrState ThreadState;
}}
