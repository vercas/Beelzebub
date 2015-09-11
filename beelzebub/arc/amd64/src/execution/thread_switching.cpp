#include <execution/thread_switching.hpp>
#include <system\cpu.hpp>
#include <debug.hpp>
#include <string.h>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::System;

/******************
    Thread class
*******************/

/*  Operations  */

Handle Thread::SwitchTo(Thread * const other, ThreadState * const dest)
{
    Handle res;

    Process * thisProc = this->Owner;
    Process * otherProc = other->Owner;

    msg("++ ");

    int_cookie_t int_cookie = Cpu::PushDisableInterrupts();

    msg("A");

    if (thisProc != otherProc)
    {
        msg("1");

        res = thisProc->SwitchTo(otherProc);

        if (!res.IsOkayResult())
        {
            Cpu::RestoreInterruptState(int_cookie);

            return res;
        }

        msg("2");
    }

    Cpu::SetActiveThread(other);

    msg("B");

    //SwitchThread(&this->KernelStackPointer, other->KernelStackPointer);

    *dest = other->State;
    //memcpy(dest, &other->State, sizeof(ThreadState));

    msg("C");

    Cpu::RestoreInterruptState(int_cookie);
    //  Doing this (restoring the interrupt state after switching BACK to
    //  this thread) makes me question my sanity and the integrity of the
    //  spacetime fabric in our perceptible dimensions of reality.

    //  The thing is, interrupts should have the same state before and
    //  after switching. I hope.

    msg(" ++");

    return HandleResult::Okay;
}
