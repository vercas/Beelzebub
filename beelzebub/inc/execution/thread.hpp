#pragma once

#include <execution/process.hpp>
#include <execution/thread_state.hpp>

namespace Beelzebub { namespace Execution
{
    typedef void (*ThreadEntryPointFunction)(void * const arg);

    /**
     *  A unit of execution.
     */
    class Thread
    {
    public:

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
