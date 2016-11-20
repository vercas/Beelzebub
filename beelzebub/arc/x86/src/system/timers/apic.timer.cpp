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

#include <system/timers/apic.timer.hpp>
#include <system/timers/pit.hpp>
#include <system/interrupt_controllers/lapic.hpp>
#include <system/cpu.hpp>
#include <synchronization/atomic.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;
using namespace Beelzebub::System::Timers;

/******************
    Calibration
******************/

static constexpr uint32_t const PitFrequency = 14551;

enum CalibrationStatus { Ongoing, Successful, Failed };

static int Stage = 0;
static constexpr int const StageCount = 100;
static Atomic<CalibrationStatus> Status;
static size_t FirstCounter, AverageCounter;

static __cold void PitTickIrqHandler(INTERRUPT_HANDLER_ARGS_FULL)
{
    if likely(Stage < StageCount)
    {
        if unlikely(Stage == 0)
            FirstCounter = Lapic::ReadRegister(LapicRegister::TimerCurrentCount);

        ++Stage;
    }
    else
    {
        size_t current = Lapic::ReadRegister(LapicRegister::TimerCurrentCount);

        //  Turn the accumulated counters into a proper average.
        AverageCounter = (FirstCounter - current) / StageCount;

        //  If calibration hasn't failed by the last stage, then it has succeeded.
        CalibrationStatus st = Ongoing;
        Status.CmpXchgStrong(st, Successful);

        //  Make sure this will not run again.
        Pit::SetHandler();
    }

    END_OF_INTERRUPT();
}

static __cold void CalibratorIrqHandler(INTERRUPT_HANDLER_ARGS_FULL)
{
    //  If calibration already succeeded, no reason to force failure upon it.
    CalibrationStatus st = Ongoing;
    Status.CmpXchgStrong(st, Failed);

    END_OF_INTERRUPT();
}

/****************
    Utilities
****************/

static uint32_t TranslateDivisor(unsigned int val)
{
    switch (val)
    {
    case   1: return 0xB;
    case   2: return 0x0;
    case   4: return 0x1;
    case   8: return 0x2;
    case  16: return 0x3;
    case  32: return 0x8;
    case  64: return 0x9;
    case 128: return 0xA;

    default:  return 0xFFFFFFFF;
    }
}

/**********************
    ApicTimer class
**********************/

/*  Statics  */

uint64_t ApicTimer::Frequency;
size_t ApicTimer::TicksPerMicrosecond;
uint32_t ApicTimer::Divisor;

/*  Initialization  */

void ApicTimer::Initialize(bool bsp)
{
    if likely(!bsp)
    {
        Lapic::WriteRegister(LapicRegister::TimerDivisor, TranslateDivisor(Divisor));
        
        return;
    }

    //  First, set up the vector.

    auto vec = Interrupts::Get(KnownExceptionVectors::ApicTimer);

    void const * handler = vec.GetHandler();

    if (handler == nullptr)
        vec.SetHandler(&CalibratorIrqHandler);
    else
    {
        ASSERT(handler == &CalibratorIrqHandler
            , "Wrong APIC timer calibration interrupt handler.")
            (handler);
    }

    vec.SetEnder(&Lapic::IrqEnder);

    //  Then, the PIT.

    Pit::SetHandler(&PitTickIrqHandler);
    //  Use the PIT for calibration.

    Pit::SetFrequency(PitFrequency);
    //  This should equate to a divisor of 82, and a period of ~68.724 microseconds.

    //  Then, calibrate.

    bool sufficient = false;
    unsigned int divisor;
    size_t freq, absFreq;

    for (divisor = 1; !sufficient && divisor <= 128; divisor <<= 1)
    {
        Lapic::WriteRegister(LapicRegister::TimerDivisor, TranslateDivisor(divisor));

        ApicTimer::SetInternal(0xFFFFFFFF, vec.GetVector(), false);

        Stage = 0;
        Status = Ongoing;

        withInterrupts (true)
            while (Status == Ongoing) { }
        //  Now, wait for the counter to change.

        if (Status == Failed)
            continue;
        //  Upon failure, the divisor needs to be increased.

        if (AverageCounter < Pit::Period)
            break;
        //  No point in testing further if microsecond precision cannot be achieved.

        freq = AverageCounter * PitFrequency;
        absFreq = freq * divisor;

        if (freq < 0xFFFFFFFFUL)
            sufficient = true;
        //  If the frequency (number of APIC timer ticks in a second) fits in the
        //  register, this divisor is sufficient.
    }

    Pit::SetHandler();

    Stop();
    //  No more timing.

    ASSERT(sufficient);

    Frequency = absFreq;
    TicksPerMicrosecond = (freq + (400000 / divisor)) / 1000000;
    Divisor = divisor;
}

/*  Operation  */

void ApicTimer::SetCount(uint32_t count)
{
    Lapic::WriteRegister(LapicRegister::TimerInitialCount, count);
}

uint32_t ApicTimer::GetCount()
{
    return Lapic::ReadRegister(LapicRegister::TimerCurrentCount);
}

void ApicTimer::Stop()
{
    Lapic::WriteRegister(LapicRegister::TimerInitialCount, 0);
}

void ApicTimer::SetInternal(uint32_t count, uint8_t interrupt, bool periodic, bool mask)
{
    Lapic::WriteRegister(LapicRegister::TimerLvt,
        ApicTimerLvt(0)
        .SetVector(interrupt)
        .SetMode(periodic ? ApicTimerMode::Periodic : ApicTimerMode::OneShot)
        .SetMask(mask)
        .Value);

    Lapic::WriteRegister(LapicRegister::TimerInitialCount, count);
}
