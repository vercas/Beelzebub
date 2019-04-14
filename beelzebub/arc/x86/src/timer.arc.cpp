/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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

#include "timer.hpp"
#include "irqs.hpp"
#include "system/timers/apic.timer.hpp"
#include "system/interrupt_controllers/lapic.hpp"
#include "system/cpuid.hpp"
#include <beel/sync/smp.lock.hpp>
#include <string.h>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;
using namespace Beelzebub::System::Timers;

/****************
    Internals
****************/

struct TimerEntry
{
    TimedFunctionVoid Function;
    void * Cookie;
    uint32_t Time;
};

static __thread uint_fast16_t MyTimersCount = 0;
static __thread TimerEntry MyTimers[Timer::Count];

static __hot void TimerIrqHandler(InterruptContext const * context, void * cookie)
{
    (void)context;
    (void)cookie;

    auto timersCount = MyTimersCount;

    if likely(timersCount > 0)
    {
        TimerEntry const entry = MyTimers[0];

        MyTimersCount = --timersCount;
        //  First timer is popped.

        if (timersCount > 0)
        {
            memmove(&(MyTimers[0]), &(MyTimers[1]), timersCount * sizeof(TimerEntry));
            //  Shifts the remaining items.

            uint32_t next = MyTimers[0].Time;

            if unlikely(next == 0)
                next = 1;
            
            ApicTimer::SetCount(next);

            //  It is VITAL that the timer is used again to guarantee that the
            //  function runs on the same core again, because the function could
            //  enable interrupts and get pre-empted or something, and end up
            //  finishing the interrupt on another core.
        }

        return entry.Function(entry.Cookie);
    }
}

static SmpLock InitLock {};
static bool Initialized = false;

static InterruptHandlerNode ApicTimerNode { &TimerIrqHandler, nullptr, Irqs::HighPriority };

/******************
    Timer class
******************/

/*  Initialization  */

void Timer::Initialize()
{
    MyTimersCount = 0;

    ApicTimer::OneShot(0, (uint8_t)Irqs::ApicTimer.Value, false);

    //  A lock is used here because this code must only be executed once, and
    //  other cores should wait for it to finish.

    withLock (InitLock)
    {
        if (Initialized)
            return;

        Initialized = true;
    }

    ASSERT(ApicTimerNode.Subscribe(Irqs::ApicTimer) == IrqSubscribeResult::Success);

    auto const regres = Lapic::Ender.Register(Irqs::ApicTimer);

    ASSERT(regres == IrqEnderRegisterResult::Success || regres == IrqEnderRegisterResult::AlreadyRegistered)(regres);

    uint32_t cccf, numerator, denominator, dummy;

    CpuId::Execute(0x15, denominator, numerator, cccf, dummy);

    DEBUG_TERM_ << "Core Crystal Clock Frequency: " << cccf
                << " Hz; Numerator: " << numerator << "; Denominator: "
                << denominator << "; dummy: " << dummy << Terminals::EndLine;
}

/*  Operation  */

bool Timer::Enqueue(TimeSpanLite delay, TimedFunctionVoid func, void * cookie)
{
    uint64_t ticks = delay.Value * ApicTimer::TicksPerMicrosecond;

    DEBUG_TERM_ << "Enqueuing timer for " << ticks << " ticks." << Terminals::EndLine;

    if unlikely(ticks > 0xFFFFFFFFULL)
    {
        MSG_("Too many ticks: %X8%n", ticks);
        return false;
    }
    else if unlikely(ticks == 0)
        ticks = 1;

    InterruptGuard<> intGuard;

    uint_fast16_t timersCount = MyTimersCount;

    if unlikely(timersCount == Timer::Count)
    {
        MSG_("Too many timers!%n");
        return false;
    }

    if likely(timersCount > 0)
    {
        //  If there are some timers already queued, the full algorithm needs to
        //  be employed.

        ticks += MyTimers[0].Time - ApicTimer::GetCount();
        //  Add the already-elapsed time to the target time.

        uint64_t lastTicks = 0;
        unsigned int i = 0;

        for (uint64_t acc = 0; i < timersCount && (acc += MyTimers[i].Time) <= ticks; ++i)
            lastTicks = acc;

        uint32_t diff = (uint32_t)(ticks - lastTicks);

        if (i < timersCount)
        {
            ::memmove(&(MyTimers[i + 1]), &(MyTimers[i]), (timersCount - i) * sizeof(TimerEntry));
            //  Shift forward the elements after the insertion point.

            MyTimers[i + 1].Time -= diff;
            //  Adjust timer after insertion point.
        }

        if (i == 0)
            ApicTimer::SetCount(diff);

        MyTimers[i].Time = diff;
        MyTimers[i].Function = func;
        MyTimers[i].Cookie = cookie;

        ++MyTimersCount;
    }
    else
    {
        //  No timers means a simple insertion at the first position.

        ApicTimer::SetCount((uint32_t)ticks);

        MyTimers[0].Time = (uint32_t)ticks;
        MyTimers[0].Function = func;
        MyTimers[0].Cookie = cookie;

        MyTimersCount = 1;
    }

    return true;
}
