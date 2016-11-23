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

#ifdef __BEELZEBUB__TEST_MAILBOX

#include <tests/mailbox.hpp>
#include <mailbox.hpp>
#include <cores.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

static constexpr size_t const PingPongCount = 200000;

struct PingPongState
{
    Synchronization::Atomic<uint64_t> Acc;
    uint64_t Min, Max;
    Synchronization::Atomic<size_t> Pings;
    Synchronization::Atomic<bool> Done, Ponged;
};

static __startup void TestFunc(void * cookie)
{
    CpuData * const data = Cpu::GetData();

    DEBUG_TERM_ << "IPI received on core " << data->Index << ": " << ((uintptr_t)cookie) << "." << EndLine;

    if ((uintptr_t)cookie == 1)
    {
        ALLOCATE_MAIL_BROADCAST(m1, &TestFunc, reinterpret_cast<void *>(Cores::GetCount() * 10000 + data->Index));
        m1.Post();
    }
}

static __startup void Pinger(void * cookie);
static __startup void Ponger(void * cookie);

void Pinger(void * cookie)
{
    PingPongState * const pps = reinterpret_cast<PingPongState *>(cookie);

    assert(!Interrupts::AreEnabled());

    assert(!pps->Ponged);
    //  Should've been cleared already.

    if likely(pps->Pings < PingPongCount)
    {
        ++pps->Pings;

        COMPILER_MEMORY_BARRIER();
        uint64_t const now = CpuInstructions::Rdtsc();
        COMPILER_MEMORY_BARRIER();

        ALLOCATE_MAIL(pong, 1, &Ponger, cookie);
        pong.Links[0] = MailboxEntryLink(1);
        pong.Post(false);

        while (!pps->Ponged) { /* Busy-wait */ }

        COMPILER_MEMORY_BARRIER();
        uint64_t const again = CpuInstructions::Rdtsc();
        COMPILER_MEMORY_BARRIER();

        uint64_t const time = (again - now);
        pps->Acc += time;

        if (time < pps->Min)
            pps->Min = time;
        if (time > pps->Max)
            pps->Max = time;

        pps->Ponged = false;
    }
    else
        pps->Done = true;
}

void Ponger(void * cookie)
{
    PingPongState * const pps = reinterpret_cast<PingPongState *>(cookie);

    pps->Ponged = true;

    ALLOCATE_MAIL(ping, 1, &Pinger, cookie);
    ping.Links[0] = MailboxEntryLink(0);
    ping.Post(false);
}

void TestMailbox()
{
    ALLOCATE_MAIL_BROADCAST(m1, &TestFunc, reinterpret_cast<void *>(1));
    m1.Post();

    ALLOCATE_MAIL_BROADCAST(m2, &TestFunc, reinterpret_cast<void *>(2));
    ALLOCATE_MAIL_BROADCAST(m3, &TestFunc, reinterpret_cast<void *>(3));
    m2.Post();
    m3.Post();

    PingPongState pps {{0}, 0xFFFFFFFFFFFFFFFFULL, 0, {0}, {false}, {false}};

    withInterrupts (false)
        Pinger(&pps);

    while (!pps.Done) CpuInstructions::DoNothing();

    uint64_t Avg = pps.Acc / pps.Pings;

    DEBUG_TERM_
        << "Direct mail latency: AVG "
        << Avg << "; MIN " << pps.Min << "; MAX " << pps.Max << EndLine;
}

#endif
