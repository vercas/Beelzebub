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

#include <system/timers/pit.hpp>
#include <system/io_ports.hpp>
#include <system/cpu.hpp>   //  Only used for task switching right now...
#include <kernel.hpp>
#include <debug.hpp>
#include <_print/isr.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::System::Timers;

/****************
    Pit class
****************/

/*  Statics  */

Atomic<size_t> Pit::Counter {0};

/*  IRQ Handler  */

void Pit::IrqHandler(INTERRUPT_HANDLER_ARGS)
{
    ++Counter;

    if (CpuDataSetUp && Scheduling)
    {
        Thread * const activeThread = Cpu::GetData()->ActiveThread;

        if (activeThread != nullptr && activeThread->Next != activeThread)
        {
            activeThread->State = *state;

            // if (activeThread == &BootstrapThread)
            //     msg("(( RIP = %Xp ))", state->RIP);

            // msg("PRE-SWITCH ");
            // PrintToDebugTerminal(state);
            // msg("%n");

            // msg("(( AT=%Xp; N=%Xp; P=%Xp; BST=%B ))%n"
            //     , activeThread, activeThread->Next, activeThread->Previous
            //     , activeThread == &BootstrapThread);

            activeThread->SwitchToNext(state);

            // msg("%nPOST-SWITCH ");
            // PrintToDebugTerminal(state);
            // msg("%n");
        }
    }

    END_OF_INTERRUPT();
}

/*  Initialization  */

void Pit::SetFrequency(uint32_t & freq)
{
    if (freq < MinimumFrequency)
        freq = MinimumFrequency;

    DividerFrequency divfreq = GetRealFrequency(freq);
    freq = divfreq.Frequency;

    Io::Out8(0x40, (uint8_t)(divfreq.Divider     ));  //  Low byte
    Io::Out8(0x40, (uint8_t)(divfreq.Divider >> 8));  //  High byte
}

void Pit::SendCommand(PitCommand const cmd)
{
    Io::Out8(0x43, cmd.Value);
}
