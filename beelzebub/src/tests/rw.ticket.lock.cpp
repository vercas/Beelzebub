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

#ifdef __BEELZEBUB__TEST_RW_TICKETLOCK

#include "tests/rw.ticket.lock.hpp"
#include "cores.hpp"
#include "kernel.hpp"
#include <beel/sync/rw.ticket.lock.hpp>

#include <debug.hpp>

#define PRINT

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

static constexpr size_t const WriterAcquisitionCount = 1'000'00;

Barrier RwTicketLockTestBarrier;

#define SYNC RwTicketLockTestBarrier.Reach()

static RwTicketLock tLock {};

void TestRwTicketLock(bool bsp)
{
    if (bsp) Scheduler::Postpone = true;

    SYNC;

    if (bsp) tLock.Reset();

    SYNC;

    if (bsp) tLock.AcquireAsWriter();

    SYNC;

    if (bsp)
    {
        DEBUG_TERM_ << "BSP is about to release..." << EndLine;

        for (size_t volatile i = 0; i < 10000000; ++i) { CpuInstructions::DoNothing(); }

        tLock.ReleaseAsWriter();

        DEBUG_TERM << "GG";
    }
    else
    {
        DEBUG_TERM_ << "AP is about to attempt acquiring as reader..." << EndLine;

        tLock.AcquireAsReader();

        DEBUG_TERM << "READER";
    }

    SYNC;

    if (bsp) ASSERT(!tLock.TryAcquireAsWriter());

    SYNC;

    if (!bsp)
        tLock.ReleaseAsReader();

    SYNC;

    tLock.AcquireAsWriter();
    
    MSG("WRITER");

    tLock.ReleaseAsWriter();

    SYNC;

    if (bsp) DEBUG_TERM << EndLine;

    SYNC;

#ifdef PRINT
    uint64_t perfStart = 0, perfEnd = 0;

    perfStart = CpuInstructions::Rdtsc();
#endif

    for (size_t i = WriterAcquisitionCount; i > 0; --i)
    {
        tLock.AcquireAsWriter();

        // for (size_t j = 3; j > 0; --j)
        //     asm volatile ( "pause \n\t" );

        tLock.ReleaseAsWriter();
    }

#ifdef PRINT
    perfEnd = CpuInstructions::Rdtsc();

    SYNC;

    MSG_("Core %us did %us writer pairs in %us cycles: %us per pair.%n"
        , Cpu::GetData()->Index, WriterAcquisitionCount, perfEnd - perfStart
        , (perfEnd - perfStart + WriterAcquisitionCount / 2 + 1) / (WriterAcquisitionCount + 2));
#endif

    SYNC;

#ifdef PRINT
    perfStart = CpuInstructions::Rdtsc();
#endif

    if (bsp)
        for (size_t i = WriterAcquisitionCount; i > 0; --i)
        {
            tLock.AcquireAsWriter();

            // for (size_t j = 3; j > 0; --j)
                asm volatile ( "pause \n\t" );

            tLock.ReleaseAsWriter();
        }
    else
        for (size_t i = WriterAcquisitionCount; i > 0; --i)
        {
            tLock.AcquireAsReader();

            // for (size_t j = 3; j > 0; --j)
                asm volatile ( "pause \n\t" );

            tLock.ReleaseAsReader();
        }

#ifdef PRINT
    perfEnd = CpuInstructions::Rdtsc();

    SYNC;

    if (bsp)
        MSG_("Core %us did %us writer pairs in %us cycles: %us per pair.%n"
            , Cpu::GetData()->Index, WriterAcquisitionCount, perfEnd - perfStart
            , (perfEnd - perfStart + WriterAcquisitionCount / 2 + 1) / (WriterAcquisitionCount + 2));
    else
        MSG_("Core %us did %us reader pairs in %us cycles: %us per pair.%n"
            , Cpu::GetData()->Index, WriterAcquisitionCount, perfEnd - perfStart
            , (perfEnd - perfStart + WriterAcquisitionCount / 2 + 1) / (WriterAcquisitionCount + 2));
#endif

    SYNC;

    if (bsp) Scheduler::Postpone = false;
}

#endif
