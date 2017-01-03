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

#if defined(__BEELZEBUB__TEST_MALLOC) && !defined(__BEELZEBUB_SETTINGS_KRNDYNALLOC_NONE)

#include "tests/malloc.hpp"
#include "cores.hpp"
#include <new>

#include <beel/sync/atomic.hpp>
#include <beel/sync/spinlock.hpp>
#include <math.h>
#include <debug.hpp>

#define __BEELZEBUB__TEST_MALLOC_ASSERTIONS

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

SmpBarrier MallocTestBarrier1 {};
SmpBarrier MallocTestBarrier2 {};
SmpBarrier MallocTestBarrier3 {};

static Spinlock<> DeleteLock {};

struct TestStructure
{
    uint64_t Qwords[3];
    uint32_t Dwords[3];
    uint16_t Words[3];
    uint8_t Bytes[3];
    TestStructure * Next;
};

static constexpr size_t const RandomIterations = 10'000'000;
static constexpr size_t const CacheSize = 2048;
static Atomic<TestStructure *> Cache[CacheSize];

void TestMalloc(bool const bsp)
{
    (void)bsp;

    size_t coreIndex = Cpu::GetData()->Index;

    MallocTestBarrier2.Reset(); //  Prepare the second barrier for re-use.
    MallocTestBarrier1.Reach();

    auto getPtr = []()
    {
        TestStructure * testptr = nullptr;

        ASSERT((testptr = new (std::nothrow) TestStructure()) != nullptr);

        return testptr;
    };

    if (bsp) MSG_("Filling array.%n");

    MallocTestBarrier3.Reset();
    MallocTestBarrier2.Reach();

    TestStructure * cur = getPtr();

    for (size_t i = 0; i < CacheSize; ++i)
    {
        TestStructure * expected = nullptr;

        if ((Cache + i)->CmpXchgStrong(expected, cur))
            cur = getPtr();
    }

    MallocTestBarrier1.Reset();
    MallocTestBarrier3.Reach();

    if (bsp) MSG_("First check.%n");

    for (size_t i = 0; i < CacheSize; ++i)
        ASSERT(cur != Cache[i]);

    MallocTestBarrier2.Reset();
    MallocTestBarrier1.Reach();

    if (bsp) MSG_("Full diff check.%n");

    for (size_t i = coreIndex; i < CacheSize; i += Cores::GetCount())
        for (size_t j = i + 1; j < CacheSize; ++j)
            ASSERT(Cache[i] != Cache[j]);

    MallocTestBarrier3.Reset();
    MallocTestBarrier2.Reach();

    withLock (DeleteLock)
    {
        MSG_("Core %us deletes %Xp.%n", coreIndex, cur);

        delete cur;
    }

    cur = nullptr;

    MallocTestBarrier1.Reset();
    MallocTestBarrier3.Reach();

    if (bsp) MSG_("Randomer!.%n");

    MallocTestBarrier2.Reset();
    MallocTestBarrier1.Reach();

    coreIndex ^= 0x55U;

    for (size_t i = 0, j = 0; j < RandomIterations; ++j)
    {
    // retry:
    //     TestStructure * old = nullptr;

    //     if (Cache[i] != nullptr)
    //     {
    //         old = (Cache + i)->Xchg(old);

    //         if unlikely(old == nullptr)
    //             goto retry;
    //         //  If it became null in the meantime, retry.

    //         MSG_("Deleting %Xp from array%n", old);

    //         delete old;
    //     }
    //     else
    //     {
    //         if (cur == nullptr)
    //         {
    //             cur = getPtr();

    //             MSG_("Allocated %Xp temporary%n", cur);
    //         }

    //         if likely((Cache + i)->CmpXchgStrong(old, cur))
    //         {
    //             MSG_("Deleting %Xp from array%n", cur);

    //             cur = nullptr;
    //         }
    //         else
    //         {
    //             //  If it became non-null in the meantime, retry.

    //             goto retry;
    //         }
    //     }

        cur = getPtr();
        TestStructure * old = (Cache + i)->Xchg(cur);

        delete old;

        i += coreIndex;

        while (i >= CacheSize)
            i -= CacheSize;
    }

    coreIndex ^= 0x55U;

    MallocTestBarrier3.Reset();
    MallocTestBarrier2.Reach();
}

#endif
