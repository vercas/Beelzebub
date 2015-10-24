#pragma once

#include <execution/process.hpp>
#include <execution/thread_state.hpp>

namespace Beelzebub { namespace Execution
{
    typedef void * (*ThreadEntryPointFunction)(void * const arg);

    /**
     *  A unit of execution.
     */
    class Thread
    {
    public:

        /*  Constructors  */

        Thread() = default;
        Thread(Thread const &) = delete;
        Thread & operator =(Thread const &) = delete;

        /*  Operations  */

        __hot __bland Handle SwitchTo(Thread * const other, ThreadState * const dest);    //  Implemented in architecture-specific code.
        __bland Handle SwitchToNext(ThreadState * const dest) { return this->SwitchTo(this->Next, dest); }

        /*  Stack  */

        uintptr_t KernelStackTop;
        uintptr_t KernelStackBottom;

        uintptr_t KernelStackPointer;

        /*  State  */

        ThreadState State;

        /*  Linkage  */

        Thread * Previous;
        Thread * Next;

        __bland Handle IntroduceNext(Thread * const other);

        //  TODO: Eventually implement a proper scheduler and drop the linkage system.

        /*  Parameters  */

        ThreadEntryPointFunction EntryPoint;

        /*  Hierarchy  */

        Process * Owner;
    };
}}
