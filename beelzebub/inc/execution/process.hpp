#pragma once

#include <handles.h>
#include <memory\manager.hpp>

using namespace Beelzebub::Memory;

namespace Beelzebub { namespace Execution
{
    /**
     *  A unit of execution.
     */
    class Process
    {
    public:

        /*  Constructors  */

        Process() = default;
        Process(Process const &) = delete;
        Process & operator =(const Process &) = delete;

        /*  Operations  */

        __hot __bland Handle SwitchTo(Process * const other);

        /*  Stack  */

        MemoryManager * VAS;
    };
}}
