/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


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

#include <utils/wait.hpp>
#include <system/timers/pit.hpp>
#include <system/cpu_instructions.hpp>
#include <beel/interrupt.state.hpp>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::System::Timers;
using namespace Beelzebub::Utils;

void Utils::Wait(uint64_t const microseconds)
{
    size_t difference = RoundUp(microseconds, Pit::Period) / Pit::Period;
    //  Round up to the length of a timer tick in microseconds, then get the
    //  number of ticks.

    size_t counterStart = Pit::Counter.Load();

    withInterrupts (true)
        do
        {
            CpuInstructions::Halt();
        } while (Pit::Counter.Load() - counterStart < difference);
        //  Yes, the CPU can be halted currently, because only the BSP uses these.
}

bool Utils::Wait(uint64_t const microseconds, PredicateFunction0 const pred)
{
    if (pred())
        return true;
    //  Eh, just checkin'?

    size_t difference = RoundUp(microseconds, Pit::Period) / Pit::Period;
    //  Round up to the length of a timer tick in microseconds, then get the
    //  number of ticks.

    size_t counterStart = Pit::Counter.Load();

    withInterrupts (true)
        do
        {
            CpuInstructions::Halt();

            if (pred())
                return true;
        } while (Pit::Counter.Load() - counterStart < difference);

    return pred();
}
