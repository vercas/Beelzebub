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
#include "system/nmi.hpp"
#include "utils/stack_walk.hpp"

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

/****************
    Internals
****************/

static Atomic<size_t> Assignee {0};
static SmpLock PrintLock {};

static Atomic<size_t> Left {0};

static __hot void WatchdogNmiHandler(INTERRUPT_HANDLER_ARGS_FULL)
{
    (void)ender;
    (void)handler;
    (void)vector;

    if (Left.Load() == 0)
        return;

    withLock (PrintLock)
    {
        MSG("%n$%us:%Xp|%Xs$%n", Cpu::GetData()->Index, state->RIP, state->RAX);

        uintptr_t stackPtr = state->RSP;
        uintptr_t const stackEnd = RoundUp(stackPtr, PageSize.Value);

        if ((stackPtr & (sizeof(size_t) - 1)) != 0)
        {
            MSG("Stack pointer was not a multiple of %us! (%Xp)%n"
                , sizeof(size_t), stackPtr);

            stackPtr &= ~((uintptr_t)(sizeof(size_t) - 1));
        }

        bool odd;
        for (odd = false; stackPtr < stackEnd; stackPtr += sizeof(size_t), odd = !odd)
        {
            MSG("%X2|%Xp|%Xs|%s"
                , (uint16_t)(stackPtr - state->RSP)
                , stackPtr
                , *((size_t const *)stackPtr)
                , odd ? "\r\n" : "\t");
        }

        if (odd) MSG("%n");

        Utils::StackFrame stackFrame;

        if (stackFrame.LoadFirst(state->RSP, state->RBP, state->RIP))
        {
            do
            {
                MSG("[Func %Xp; Stack top %Xp + %us]%n"
                    , stackFrame.Function, stackFrame.Top, stackFrame.Size);

            } while (stackFrame.LoadNext());
        }
    }

    --Left;
}

static Nmi::HandlerNode NmiEntry { &WatchdogNmiHandler };

static __hot void WatchdogTimerHandler(IsrState * const state, void * cookie)
{
    (void)state;
    (void)cookie;

    MSG_("&%us&", Cpu::GetData()->Index);

    Left.Store(Cores::GetCount());
    Nmi::Broadcast();

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

    Nmi::AddHandler(&NmiEntry);

    Timer::Enqueue(1secs_l, WatchdogTimerHandler);

    MSG_("Core %us is the assigned watchdog.%n", Cpu::GetData()->Index);

    return true;
}

/*  Check  */

bool Watchdog::AmIInCharge()
{
    return Cpu::GetData()->Index == Assignee.Load();
}
