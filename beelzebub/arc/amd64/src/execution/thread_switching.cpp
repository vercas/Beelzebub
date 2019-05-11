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

#include "execution/thread.hpp"
#include "system/cpu.hpp"
#include "system/fpu.hpp"
#include "system/syscalls.hpp"

#include <string.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::System;

/******************
    Thread class
*******************/

/*  Operations  */

Handle Thread::SwitchTo(Thread * const other, GeneralRegisters64 * const dest)
{
    Handle res;

    Process * const thisProc = this->Owner;
    Process * const otherProc = other->Owner;

    //msg("++ ");

    this->State.GeneralRegisters = *dest;

    InterruptGuard<> intGuard;

    //msg("A");

    if (thisProc != otherProc)
    {
        //msg("1");

        res = thisProc->SwitchTo(otherProc);

        if (!res.IsOkayResult())
            return res;

        //msg("2");
    }

    if (this->ExtendedState != nullptr)
    {
        Fpu::SaveState(this->ExtendedState);
        //  Save the state now. This may change later.
    }

    auto cpuData = Cpu::GetData();

    cpuData->ActiveThread = other;
    cpuData->ActiveProcess = otherProc;
    cpuData->EmbeddedTss.Rsp[0] = other->KernelStackTop;
    SyscallStack = other->KernelStackTop;

    //msg("B");

    *dest = other->State.GeneralRegisters;

    if (other->ExtendedState != nullptr)
    {
        if (this->ExtendedState == nullptr)
            CpuInstructions::Clts();
        //  Need this now.

        if (cpuData->LastExtendedStateThread != other)
        {
            //  So, the last thread whose extended state was used isn't this one.

            Fpu::LoadState(other->ExtendedState);
            //  Load new thread's extended state now. Don't waste cycles with yet
            //  another exception.

            if (this->ExtendedState != nullptr)
                CpuInstructions::Clts();
        }

        cpuData->LastExtendedStateThread = nullptr;
    }
    else if (this->ExtendedState != nullptr)
    {
        //  So, old thread had a state but the new one doesn't.

        Cpu::SetCr0(Cpu::GetCr0().SetTaskSwitched(true));
        //  This makes the FPU & SSE unusable.

        cpuData->LastExtendedStateThread = this;
        //  Remember the last thread whose extended state was used.
    }

    //msg(" ++");

    return HandleResult::Okay;
}
