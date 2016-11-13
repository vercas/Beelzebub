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

#include <timer.hpp>
#include <system/timers/apic.timer.hpp>
#include <system/cpu.hpp>
#include <synchronization/atomic.hpp>
#include <synchronization/spinlock.hpp>
#include <kernel.hpp>
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

static __cold void TimerIrqHandler(INTERRUPT_HANDLER_ARGS_FULL)
{
    if likely(CpuDataSetUp)
    {
        CpuData * const data = Cpu::GetData();

        DEBUG_TERM << "TIMER on core " << data->Index << Terminals::EndLine;

        auto timersCount = data->TimersCount;

        if (timersCount > 0)
        {
            TimerEntry const entry = data->Timers[0];

            data->TimersCount = --timersCount;
            //  First timer is popped.

            if (timersCount > 0)
            {
                memmove(&(data->Timers[0]), &(data->Timers[1]), timersCount * sizeof(TimerEntry));
                //  Shifts the remaining items.

                ApicTimer::SetCount(data->Timers[0].Time);
            }

            if (entry.Function != nullptr)
                entry.Function(state, entry.Cookie);
        }
    }

    END_OF_INTERRUPT();
}

static Spinlock<> InitLock {};
static bool Initialized = false;

/******************
    Timer class
******************/

/*  Initialization  */

void Timer::Initialize()
{
    auto const vec = Interrupts::Get(KnownExceptionVectors::ApicTimer);
    //  Unique.

    CpuData * const data = Cpu::GetData();

    data->TimersCount = 0;
    
    ApicTimer::OneShot(0, vec.GetVector());

    //  A lock is used here because this code must only be executed once, and
    //  other cores should wait for it to finish.

    withLock (InitLock)
    {
        if (Initialized)
            return;

        vec.SetHandler(&TimerIrqHandler);

        Initialized = true;
    }
}

/*  Operation  */

bool Timer::Enqueue(TimeSpanLite delay, TimedFunction func, void * cookie)
{
    InterruptGuard<> intGuard;

    CpuData * const data = Cpu::GetData();
    uint_fast16_t timersCount = data->TimersCount;

    if unlikely(data->TimersCount == Timer::Count)
        return false;

    uint64_t ticks = delay.Value * ApicTimer::TicksPerMicrosecond;

    if unlikely(ticks > 0xFFFFFFFFULL)
        return false;

    uint32_t lastTicks = 0;
    unsigned int i;

    for (i = 0; i < timersCount && data->Timers[i].Time <= ticks; ++i)
        lastTicks = data->Timers[i].Time;

    uint32_t diff = (uint32_t)(ticks - lastTicks);

    if (i < timersCount)
        memmove(&(data->Timers[i + 1]), &(data->Timers[i]), (timersCount - i) * sizeof(TimerEntry));
    //  Shift forward the elements after the insertion point.

    data->Timers[i].Time = diff;
    data->Timers[i].Function = func;
    data->Timers[i].Cookie = cookie;

    if (i == 0)
        ApicTimer::SetCount(diff);

    for (++i; i <= timersCount; ++i)
        data->Timers[i].Time -= diff;

    ++data->TimersCount;

    return true;
}
