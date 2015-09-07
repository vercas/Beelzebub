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

        /*  Stack  */

        MemoryManager * VAS;
    };
}}
