#include <execution/thread.hpp>
#include <system/cpu.hpp>
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

    Process * const thisProc = this->Owner;
    Process * const otherProc = other->Owner;

    msg("++ ");

    int_cookie_t const int_cookie = Interrupts::PushDisable();

    msg("A");

    if (thisProc != otherProc)
    {
        msg("1");

        res = thisProc->SwitchTo(otherProc);

        if (!res.IsOkayResult())
        {
            Interrupts::RestoreState(int_cookie);

            return res;
        }

        msg("2");
    }

    Cpu::SetActiveThread(other);

    msg("B");

    //SwitchThread(&this->KernelStackPointer, other->KernelStackPointer);

    auto interruptVector = dest->Vector;
    auto errorCode = dest->ErrorCode;

    *dest = other->State;
    //memcpy(dest, &other->State, sizeof(ThreadState));

    dest->ErrorCode = errorCode;
    dest->Vector = interruptVector;

    msg("C");

    Interrupts::RestoreState(int_cookie);
    //  Doing this (restoring the interrupt state after switching BACK to
    //  this thread) makes me question my sanity and the integrity of the
    //  spacetime fabric in our perceptible dimensions of reality.

    //  The thing is, interrupts should have the same state before and
    //  after switching. I hope.

    msg(" ++");

    return HandleResult::Okay;
}
