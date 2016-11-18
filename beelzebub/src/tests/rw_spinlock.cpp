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

#include <tests/rw_spinlock.hpp>
#include <synchronization/rw_spinlock.hpp>
#include <cores.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

SmpBarrier RwSpinlockTestBarrier1 {};
SmpBarrier RwSpinlockTestBarrier2 {};
SmpBarrier RwSpinlockTestBarrier3 {};

RwSpinlock tLock {};

void TestRwSpinlock(bool bsp)
{
    RwSpinlockTestBarrier1.Reach();

    if (bsp) tLock.Reset();

    RwSpinlockTestBarrier2.Reach();
    RwSpinlockTestBarrier1.Reset();

    ASSERT(!tLock.HasWriter());
    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    RwSpinlockTestBarrier3.Reach();
    RwSpinlockTestBarrier2.Reset();

    if (bsp) tLock.AcquireAsWriter();

    RwSpinlockTestBarrier1.Reach();
    RwSpinlockTestBarrier3.Reset();

    ASSERT(tLock.HasWriter());
    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    RwSpinlockTestBarrier2.Reach();
    RwSpinlockTestBarrier1.Reset();

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

    RwSpinlockTestBarrier3.Reach();
    RwSpinlockTestBarrier2.Reset();

    ASSERT(!tLock.HasWriter());
    ASSERT_EQ("%us", Cores::GetCount() - 1UL, tLock.GetReaderCount());

    RwSpinlockTestBarrier1.Reach();
    RwSpinlockTestBarrier3.Reset();

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

    RwSpinlockTestBarrier2.Reach();
    RwSpinlockTestBarrier1.Reset();

    ASSERT(tLock.HasWriter());
    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    RwSpinlockTestBarrier3.Reach();
    RwSpinlockTestBarrier2.Reset();

    ASSERT(!tLock.TryAcquireAsReader());
    //  All must fail, including the writer.

    RwSpinlockTestBarrier1.Reach();
    RwSpinlockTestBarrier3.Reset();

    if (Cpu::GetData()->Index == 1)
    {
        tLock.DowngradeToReader();
        ASSERT_EQ("%us", 1UL, tLock.GetReaderCount());
    }
    else if (bsp)
    {
        ASSERT(!tLock.TryAcquireAsWriter());
    }

    RwSpinlockTestBarrier2.Reach();
    RwSpinlockTestBarrier1.Reset();

    ASSERT(!tLock.HasWriter());

    RwSpinlockTestBarrier3.Reach();
    RwSpinlockTestBarrier2.Reset();

    if (Cpu::GetData()->Index == 1)
    {
        tLock.ReleaseAsReader();
    }

    RwSpinlockTestBarrier1.Reach();
    RwSpinlockTestBarrier3.Reset();

    ASSERT(!tLock.HasWriter());
    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    RwSpinlockTestBarrier2.Reach();
    RwSpinlockTestBarrier1.Reset();

    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    tLock.AcquireAsWriter();
    ASSERT(tLock.HasWriter());
    
    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    MSG("WRITER");

    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());
    
    ASSERT(tLock.HasWriter());
    tLock.ReleaseAsWriter();

    ASSERT_EQ("%us", 0UL, tLock.GetReaderCount());

    RwSpinlockTestBarrier3.Reach();
    RwSpinlockTestBarrier2.Reset();

    // RwSpinlockTestBarrier1.Reach();
    // RwSpinlockTestBarrier3.Reset();

    // RwSpinlockTestBarrier2.Reach();
    // RwSpinlockTestBarrier1.Reset();

    // RwSpinlockTestBarrier3.Reach();
    // RwSpinlockTestBarrier2.Reset();

    // RwSpinlockTestBarrier1.Reach();
    // RwSpinlockTestBarrier3.Reset();

    // RwSpinlockTestBarrier2.Reach();
    // RwSpinlockTestBarrier1.Reset();
}

#endif
