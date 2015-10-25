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

        __bland inline Process()
            : Memory( nullptr)
        {

        }

        Process(Process const &) = delete;
        Process & operator =(Process const &) = delete;

        __bland inline Process(MemoryManager * const memory)
            : Memory( memory)
        {

        }

        /*  Operations  */

        __hot __bland Handle SwitchTo(Process * const other);

        /*  Stack  */

        MemoryManager * const Memory;
    };
}}
