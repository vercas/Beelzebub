#include <execution/thread_switching.hpp>
#include <system\cpu.hpp>

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

    int_cookie_t int_cookie = Cpu::PushDisableInterrupts();

    if (thisProc != otherProc)
    {
        res = thisProc->SwitchTo(otherProc);

        if (!res.IsOkayResult())
        {
            Cpu::RestoreInterruptState(int_cookie);

            return res;
        }
    }

    Cpu::SetActiveThread(other);

    SwitchThread(&this->KernelStackPointer, other->KernelStackPointer);

    Cpu::RestoreInterruptState(int_cookie);
    //  Doing this (restoring the interrupt state after switching BACK to
    //  this thread) makes me question my sanity and the integrity of the
    //  spacetime fabric in our perceptible dimensions of reality.

    //  The thing is, interrupts should have the same state before and
    //  after switching. I hope.

    return HandleResult::Okay;
}
