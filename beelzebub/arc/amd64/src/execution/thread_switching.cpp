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

#include <execution/thread.hpp>
#include <system/cpu.hpp>
#include <system/fpu.hpp>

#include <string.h>
#include <debug.hpp>

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

    //msg("++ ");

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
        //  Save state
    }

    auto cpuData = Cpu::GetData();

    cpuData->ActiveThread = other;
    cpuData->EmbeddedTss.Rsp[0] = other->KernelStackTop;
    cpuData->SyscallStack = other->KernelStackTop;

    //msg("B");

    *dest = other->State;

    if (other->ExtendedState != nullptr)
    {
        //  Restore state

        if (this->ExtendedState != nullptr)
            CpuInstructions::Clts();
    }
    else
    {
        //  Do... Something?
    }

    //msg(" ++");

    return HandleResult::Okay;
}
