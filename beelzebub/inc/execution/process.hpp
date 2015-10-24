#pragma once

#include <memory/manager.hpp>
#include <handles.h>

using namespace Beelzebub::Memory;

namespace Beelzebub { namespace Execution
{
    /**
     *  A unit of isolation.
     */
    class Process
    {
    public:

        /*  Constructors  */

        Process() = default;
        Process(Process const &) = delete;
        Process & operator =(Process const &) = delete;

        /*  Operations  */

        __hot __bland Handle SwitchTo(Process * const other);

        /*  Stack  */

        MemoryManager * VAS;
    };
}}
