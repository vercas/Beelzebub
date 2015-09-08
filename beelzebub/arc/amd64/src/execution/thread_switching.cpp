#include <execution/thread_switching.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

/******************
    Thread class
*******************/

/*  Operations  */

Handle Thread::SwitchTo(Thread * const other)
{
    Handle res;

    Process * thisProc = this->Owner;
    Process * otherProc = other->Owner;

    if (thisProc != otherProc)
    {
        res = thisProc->SwitchTo(otherProc);

        if (!res.IsOkayResult())
            return res;
    }

    SwitchThread(&this->KernelStackPointer, other->KernelStackPointer);

    //  This function will return when the thread is re-entered! :D

    return HandleResult::Okay;
}
