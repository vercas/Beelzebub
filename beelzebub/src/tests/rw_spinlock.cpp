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

#ifdef __BEELZEBUB__TEST_RW_SPINLOCK

#include "tests/rw_spinlock.hpp"
#include "cores.hpp"
#include "kernel.hpp"
#include <beel/sync/rw.spinlock.hpp>

#include <debug.hpp>

#define PRINT

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

static constexpr size_t const WriterAcquisitionCount = 1'000'00;

Barrier RwSpinlockTestBarrier;

#define SYNC RwSpinlockTestBarrier.Reach()

static RwSpinlock tLock {};

void TestRwSpinlock(bool bsp)
{
    if (bsp) Scheduling = false;

    SYNC;

    if (bsp) tLock.Reset();

    SYNC;

    ASSERT(!tLock.HasWriter());
    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    SYNC;

    if (bsp) tLock.AcquireAsWriter();

    SYNC;

    ASSERT(tLock.HasWriter());
    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

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

        MSG("READER");
    }

    SYNC;

    ASSERT(!tLock.HasWriter());
    ASSERT_EQ("%us", Cores::GetCount() - 1UL, tLock.GetReaderCount());

    SYNC;

    if (Cpu::GetData()->Index == 1)
    {
        ASSERT(!tLock.HasWriter());
        ASSERT(tLock.UpgradeToWriter());
        ASSERT(tLock.HasWriter());
    }
    else if (!bsp)
    {
        tLock.ReleaseAsReader();
        //  Allow the upgrade to occur.
    }

    SYNC;

    ASSERT(tLock.HasWriter());
    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    SYNC;

    ASSERT(!tLock.TryAcquireAsReader());
    //  All must fail, including the writer.

    SYNC;

    if (Cpu::GetData()->Index == 1)
    {
        tLock.DowngradeToReader();
        ASSERT_EQ("%us", 1UL, tLock.GetReaderCount());
    }
    else if (bsp)
    {
        ASSERT(!tLock.TryAcquireAsWriter());
    }

    SYNC;

    ASSERT(!tLock.HasWriter());

    SYNC;

    if (Cpu::GetData()->Index == 1)
    {
        tLock.ReleaseAsReader();
    }

    SYNC;

    ASSERT(!tLock.HasWriter());
    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    SYNC;

    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    tLock.AcquireAsWriter();
    ASSERT(tLock.HasWriter());
    
    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    MSG("WRITER");

    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());
    
    ASSERT(tLock.HasWriter());
    tLock.ReleaseAsWriter();

    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

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

    if (bsp) Scheduling = true;
}

#endif
