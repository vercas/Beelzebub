#pragma once

#include <execution/process.hpp>
#include <execution/thread_state.hpp>
#include <handles.h>

namespace Beelzebub { namespace Execution
{
    typedef void (*ThreadEntryPointFunction)(void * const arg);

    /**
     *  A unit of execution.
     */
    class Thread
    {
    public:

        /*  Operations  */

        __hot __bland Handle SwitchTo(Thread * const other);

        /*  Stack  */

        uintptr_t KernelStackTop;
        uintptr_t KernelStackBottom;

        uintptr_t KernelStackPointer;

        /*  Linkage  */

        Thread * Previous;
        Thread * Next;

        /*  Parameters  */

        ThreadEntryPointFunction EntryPoint;

        /*  Hierarchy  */

        Process * Owner;
    };
}}
