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

#ifdef __BEELZEBUB__TEST_INTERRUPT_LATENCY

#include <tests/interrupt_latency.hpp>

#include <debug.hpp>

static constexpr size_t const IterationCount = 200000;

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

uint64_t midInterruptTime;

void LatencyTestInterruptHandlerPartial(INTERRUPT_HANDLER_ARGS)
{
    midInterruptTime = CpuInstructions::Rdtsc();
}

void LatencyTestInterruptHandlerFull(INTERRUPT_HANDLER_ARGS_FULL)
{
    midInterruptTime = CpuInstructions::Rdtsc();
}

template<uint8_t vec>
__startup void DoTest(bool const full)
{
    midInterruptTime = 0xFFFFFFFFFFFFFFFFUL;
    uint64_t entryAcc = 0, exitAcc = 0;
    uint64_t maxEntryDur = 0, minEntryDur = 0xFFFFFFFFFFFFFFFFUL;
    uint64_t maxExitDur = 0, minExitDur = 0xFFFFFFFFFFFFFFFFUL;

    for (size_t i = 0; i < IterationCount; ++i)
    {
        COMPILER_MEMORY_BARRIER();
        uint64_t entryTime = CpuInstructions::Rdtsc();
        COMPILER_MEMORY_BARRIER();
        Interrupts::Trigger<vec>();
        COMPILER_MEMORY_BARRIER();
        uint64_t exitDur = CpuInstructions::Rdtsc() - midInterruptTime;
        COMPILER_MEMORY_BARRIER();
        uint64_t entryDur = midInterruptTime - entryTime;

        //  Yes, lots of barriers to make sure the compiler doesn't do any
        //  wanna-be smart reordering.

        entryAcc += entryDur;
        exitAcc += exitDur;

        if (entryDur < minEntryDur) minEntryDur = entryDur;
        if (exitDur  < minExitDur ) minExitDur  = exitDur;
        if (entryDur > maxEntryDur) maxEntryDur = entryDur;
        if (exitDur  > maxExitDur ) maxExitDur  = exitDur;
    }

    uint64_t avgEntryDur = entryAcc / IterationCount;
    uint64_t avgExitDur  = exitAcc  / IterationCount;

    DEBUG_TERM_
        << "Interrupt (" << (full ? "FULL" : "PARTIAL") << ") entry latency: AVG "
        << avgEntryDur << "; MIN " << minEntryDur << "; MAX " << maxEntryDur << EndLine
        << "Interrupt (" << (full ? "FULL" : "PARTIAL") << ") exit latency: AVG "
        << avgExitDur << "; MIN " << minExitDur << "; MAX " << maxExitDur << EndLine;
}

void TestInterruptLatency()
{
    InterruptGuard<false> ig;

    Interrupts::Get(0xDF).SetHandler(&LatencyTestInterruptHandlerPartial);
    Interrupts::Get(0xDE).SetHandler(&LatencyTestInterruptHandlerFull);

    DoTest<0xDF>(false);
    DoTest<0xDE>(true);
}

#endif
