/*
    Copyright (c) 2017 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#include "watchdog.hpp"
#include "cores.hpp"
#include "timer.hpp"
#include "system/interrupt_controllers/lapic.hpp"

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;

/****************
    Internals
****************/

static Atomic<size_t> Assignee {0};

static __hot __realign_stack void WatchdogIsrHandler(INTERRUPT_HANDLER_ARGS_FULL)
{
    MSG_("$%us:%Xp|%Xs$", Cpu::GetData()->Index, state->RIP, state->RAX);

    END_OF_INTERRUPT();
}

static __hot void WatchdogTimerHandler(IsrState * const state, void * cookie)
{
    (void)state;
    (void)cookie;

    MSG_("&%us&", Cpu::GetData()->Index);

    Lapic::SendIpi(LapicIcr(0)
        .SetDeliveryMode(InterruptDeliveryModes::NMI)
        .SetDestinationShorthand(IcrDestinationShorthand::AllExcludingSelf)
        .SetAssert(true));

    Timer::Enqueue(1secs_l, WatchdogTimerHandler);
}

/*********************
    Watchdog class
*********************/

/*  Initialization  */

bool Watchdog::Initialize()
{
    //  Yes, this will ignore core 0 - the bootstrap processor.

    size_t expected = 0;

    if (!Assignee.CmpXchgStrong(expected, Cpu::GetData()->Index))
        return false;

    Interrupts::Get(KnownExceptionVectors::NmiInterrupt).SetHandler(&WatchdogIsrHandler).SetEnder(&Lapic::IrqEnder);

    Timer::Enqueue(1secs_l, WatchdogTimerHandler);

    MSG_("Core %us is the assigned watchdog.%n", Cpu::GetData()->Index);

    return true;
}

/*  Check  */

bool Watchdog::AmIInCharge()
{
    return Cpu::GetData()->Index == Assignee.Load();
}
