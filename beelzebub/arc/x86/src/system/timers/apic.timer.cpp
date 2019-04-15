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

#include "system/timers/apic.timer.hpp"
#include "system/timers/pit.hpp"
#include "system/interrupt_controllers/lapic.hpp"
#include "system/cpuid.hpp"
#include "system/msrs.hpp"
#include "system/cpu_instructions.hpp"
#include <beel/sync/atomic.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;
using namespace Beelzebub::System::Timers;

/******************
    Calibration
******************/

static __cold void PitTickIrqHandler(InterruptContext const * context, void * cookie);
static __cold void CalibratorIrqHandler(InterruptContext const * context, void * cookie);

InterruptHandlerNode PitNode { &PitTickIrqHandler, nullptr, Irqs::MaxPriority };
InterruptHandlerNode LapicNode { &CalibratorIrqHandler, nullptr, Irqs::MaxPriority };

enum CalibrationStatus { Ongoing, Successful, Failed };

static int Stage = 0;
static int TotalStages = 100;
static Atomic<CalibrationStatus> Status;
static size_t FirstTicks, AverageTicksPerStage;
static size_t FirstCount, AverageCountsPerStage;

void PitTickIrqHandler(InterruptContext const * context, void * cookie)
{
    (void)context;
    (void)cookie;

    if unlikely(Stage >= TotalStages)
    {
        size_t const currentTicks = Lapic::ReadRegister(LapicRegister::TimerCurrentCount);
        size_t const currentCount = CpuInstructions::Rdtsc();

        //  Turn the accumulated ticks and count into proper averages.
        AverageTicksPerStage = (FirstTicks - currentTicks) / TotalStages;
        AverageCountsPerStage = (currentCount - FirstCount) / TotalStages;

        //  If calibration hasn't failed by the last stage, then it has succeeded.
        CalibrationStatus st = Ongoing;
        Status.CmpXchgStrong(st, Successful);
    }
    else if unlikely(Stage++ == 0)  //  Note the post-increment!
    {
        FirstTicks = Lapic::ReadRegister(LapicRegister::TimerCurrentCount);
        FirstCount = CpuInstructions::Rdtsc();
    }
}

void CalibratorIrqHandler(InterruptContext const * context, void * cookie)
{
    (void)context;
    (void)cookie;

    //  If calibration already succeeded, no reason to force failure upon it.
    CalibrationStatus st = Ongoing;
    Status.CmpXchgStrong(st, Failed);
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

uint64_t ApicTimer::Frequency, ApicTimer::TscFrequency;
size_t ApicTimer::TicksPerMicrosecond, ApicTimer::CountsPerMicrosecond;
uint32_t ApicTimer::Divisor;
bool ApicTimer::TscDeadline;

/*  Initialization  */

void ApicTimer::Initialize(bool bsp)
{
    if likely(!bsp)
    {
        Lapic::WriteRegister(LapicRegister::TimerDivisor, TranslateDivisor(Divisor));

        return;
    }

    if (BootstrapCpuid.MaxStandardValue >= 0x15)
    {
        uint32_t cccf = 1337, numerator = 1338, denominator = 1339, dummy = 1340;

        CpuId::Execute(0x15, denominator, numerator, cccf, dummy);

        DEBUG_TERM_ << "Core Crystal Clock Frequency: " << cccf
                    << " Hz; Numerator: " << numerator << "; Denominator: "
                    << denominator << "; dummy: " << dummy << Terminals::EndLine;
    }

    if (BootstrapCpuid.CheckFeature(CpuFeature::VIRTUALIZED) && BootstrapCpuid.MaxVirtualizationValue >= 0x40000010)
    {
        uint32_t tscFreq = 1341, apicFreq = 1342, ecx = 1343, edx = 1344;

        CpuId::Execute(0x40000010, tscFreq, apicFreq, ecx, edx);

        DEBUG_TERM_ << "TSC Frequency: " << tscFreq << " KHz; APIC Frequency: "
                    << apicFreq << "; ECX: " << ecx << "; EDX: " << edx << Terminals::EndLine;
    }

    //  First, set up the handlers.

    ASSERT(PitNode.Subscribe(Pit::IrqVector) == IrqSubscribeResult::Success);
    ASSERT(LapicNode.Subscribe(Irqs::ApicTimer) == IrqSubscribeResult::Success);

    ASSERT(Lapic::Ender.Register(Irqs::ApicTimer) == IrqEnderRegisterResult::Success);

    //  Then, the PIT.

    uint32_t PitFrequency = 14551;

    if (BootstrapCpuid.CheckFeature(CpuFeature::VIRTUALIZED))
    {
        PitFrequency = 100;
        TotalStages = 10;
    }

    Pit::SetFrequency(PitFrequency);
    //  This should equate to a divisor of 82, and a period of ~68.724 microseconds.

    //  Then, calibrate.
    bool sufficient = false;
    unsigned int divisor;
    size_t freq = 0, absFreq = 0, tscfrq = 0;

    for (divisor = 1; !sufficient && divisor <= 128; divisor <<= 1)
    {
        Lapic::WriteRegister(LapicRegister::TimerDivisor, TranslateDivisor(divisor));

        ApicTimer::SetInternal(0xFFFFFFFF, (uint8_t)Irqs::ApicTimer.Value, false);

        Stage = 0;
        Status = Ongoing;

        withInterrupts (true)
            while (Status == Ongoing) { }
        //  Now, wait for the counter to change.

        if (Status == Failed)
            continue;
        //  Upon failure, the divisor needs to be increased.

        if (AverageTicksPerStage < Pit::Period)
            break;
        //  No point in testing further if microsecond precision cannot be achieved.

        freq = AverageTicksPerStage * PitFrequency;
        absFreq = freq * divisor;
        tscfrq = AverageCountsPerStage * PitFrequency;

        if (freq < 0xFFFFFFFFUL)
            sufficient = true;
        //  If the frequency (number of APIC timer ticks in a second) fits in the
        //  register, this divisor is sufficient.
    }

    ASSERT(PitNode.Unsubscribe() == IrqUnsubscribeResult::Success);
    ASSERT(LapicNode.Unsubscribe() == IrqUnsubscribeResult::Success);

    Stop();
    //  No more timing.

    ASSERT(sufficient);

    Frequency = absFreq;
    TscFrequency = tscfrq;
    TicksPerMicrosecond = (freq + (400000 / divisor)) / 1000000;
    CountsPerMicrosecond = TscFrequency / 1000000;
    Divisor = divisor;

    TscDeadline = BootstrapCpuid.CheckFeature(CpuFeature::TscDeadline);

    MSGEX("APIC timer calibrated:\n\tAPIC Timer Frequency: {0}\n\tAPIC Ticks per Microsecond: {2}"
          "\n\tPIT Divisor: {4}\n\tTSC Frequency: {1}\n\tTSC Counts per Microsecond: {3}"
          "\n\tTSC Deadline: {5}\n"
        , Frequency, TscFrequency, TicksPerMicrosecond, CountsPerMicrosecond, Divisor, TscDeadline);
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
    Lapic::WriteTimerLvt(ApicTimerLvt(0)
        .SetVector(interrupt)
        .SetMode(periodic ? ApicTimerMode::Periodic : ApicTimerMode::OneShot)
        .SetMask(mask));

    Lapic::WriteRegister(LapicRegister::TimerInitialCount, count);
}
