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

#include <beel/sync/spinlock.hpp>
#include <math.h>
#include <debug.hpp>

#ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_VALLOC
#include <valloc/interface.hpp>
#endif

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

Barrier MallocTestBarrier;

#define SYNC MallocTestBarrier.Reach()

static Spinlock<> DeleteLock {};

struct TestStructure
{
    uint64_t Qwords[3];
    uint32_t Dwords[3];
    uint16_t Words[3];
    uint8_t Bytes[3];
    TestStructure * Next;

    uint64_t Dummy[100];
};

static constexpr size_t const RandomIterations = 1'00'000;
static constexpr size_t const CacheSize = 2048;
static Atomic<TestStructure *> Cache[CacheSize];
static __thread TestStructure * MyCache[CacheSize];
//  There is no space on the stack for this one.

void TestMalloc(bool const bsp)
{
    InterruptGuard<false> ig;

    size_t coreIndex = Cpu::GetData()->Index;

    auto getPtr = []()
    {
        TestStructure * testptr = nullptr;

        ASSERT((testptr = new (std::nothrow) TestStructure()) != nullptr, "Null pointer.");

#ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_VALLOC
        ASSERTX(reinterpret_cast<uintptr_t>(testptr) % 64 == (4 * sizeof(void *))
            , "Misaligned pointer.")
            ("pointer", (void *)testptr)XEND;
#endif

        return testptr;
    };

    if (bsp) MSG_("Filling array.%n");

    SYNC;

    uint64_t perfStart = 0, perfEnd = 0;

    if (bsp) perfStart = CpuInstructions::Rdtsc();

    TestStructure * cur = getPtr();

    for (size_t i = 0; i < CacheSize; ++i)
    {
        TestStructure * expected = nullptr;

        if ((Cache + i)->CmpXchgStrong(expected, cur))
            cur = getPtr();
    }

    if (bsp) perfEnd = CpuInstructions::Rdtsc();

    SYNC;

    if (bsp)
    {
        MSG_("Filling took %us cycles: %us per slot.%n"
            , perfEnd - perfStart
            , (perfEnd - perfStart + CacheSize / 2) / CacheSize);

        MSG_("First check.%n");
    }

    for (size_t i = 0; i < CacheSize; ++i)
        ASSERT(cur != Cache[i]);

    SYNC;

    if (bsp) MSG_("Full diff check.%n");

    for (size_t i = coreIndex; i < CacheSize; i += Cores::GetCount())
        for (size_t j = i + 1; j < CacheSize; ++j)
            ASSERT(Cache[i] != Cache[j]);

    SYNC;

    withLock (DeleteLock)
    {
        MSG_("Core %us deletes %Xp.%n", coreIndex, cur);

        delete cur;
    }

    cur = nullptr;

    SYNC;

    if (bsp) MSG_("Individual stability 1.%n");

    SYNC;

    perfStart = CpuInstructions::Rdtsc();

    for (size_t i = 0; i < RandomIterations; ++i)
        delete getPtr();

    perfEnd = CpuInstructions::Rdtsc();

    MSG_("Core %us did %us malloc & free pairs in %us cycles; %us cycles per pair.%n"
        , coreIndex, RandomIterations, perfEnd - perfStart, (perfEnd - perfStart + RandomIterations / 2) / RandomIterations);

    SYNC;

    if (bsp) MSG_("Individual stability 2.%n");

    for (size_t i = 0; i < CacheSize; ++i)
        MyCache[i] = nullptr;

    SYNC;

    perfStart = CpuInstructions::Rdtsc();

    for (size_t i = 0, j = 0; j < RandomIterations; ++j)
    {
        if (MyCache[i] == nullptr)
            MyCache[i] = getPtr();
        else
        {
            delete MyCache[i];
            MyCache[i] = nullptr;
        }

        if (++i == CacheSize) i = 0;
    }

    for (size_t i = 0; i < CacheSize; ++i)
        if (MyCache[i] != nullptr)
            delete MyCache[i];

    perfEnd = CpuInstructions::Rdtsc();

    MSG_("Core %us did %us malloc & free in the same order in %us cycles; %us cycles per operation.%n"
        , coreIndex, RandomIterations, perfEnd - perfStart, (perfEnd - perfStart + RandomIterations / 2) / RandomIterations);

    SYNC;

    static Atomic<TestStructure *> Syncer (nullptr);

    if (bsp)
    {
        MSG_("Syncer.%n");

        Syncer = getPtr();
    }

    SYNC;

    if (!bsp)
    {
        TestStructure * old = nullptr;

        Syncer.Xchg(old);

        if (old != nullptr)
        {
            MSG_("Core %us deletes %Xp.%n", coreIndex, old);

            delete old;
        }
    }

    SYNC;

#ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_VALLOC
    if (bsp)
    {
        MSG_("Collecting.%n");

        Valloc::CollectMyGarbage();
    }
#endif

    SYNC;

    SYNC;

    if (bsp) MSG_("Randomer!%n");

    SYNC;

    coreIndex ^= 0x55U;

    for (size_t i = 0, j = 0; j < RandomIterations; ++j)
    {
        if ((j & 4095) == 0)
            MSG_("[%us:%us]", coreIndex ^ 0x55U, j);

    retry:
        TestStructure * old = nullptr;

        if (Cache[i] != nullptr)
        {
            old = (Cache + i)->Xchg(old);

            if unlikely(old == nullptr)
                goto retry;
            //  If it became null in the meantime, retry.

            // MSG_("Deleting %Xp from array%n", old);

            delete old;
        }
        else
        {
            if (cur == nullptr)
            {
                cur = getPtr();

                // MSG_("Allocated %Xp temporary%n", cur);
            }

            if likely((Cache + i)->CmpXchgStrong(old, cur))
            {
                // MSG_("Deleting %Xp from array%n", cur);

                cur = nullptr;
            }
            else
            {
                //  If it became non-null in the meantime, retry.

                goto retry;
            }
        }

        // cur = getPtr();
        // TestStructure * old = (Cache + i)->Xchg(cur);

        // delete old;

        i += coreIndex;

        while (i >= CacheSize)
            i -= CacheSize;
    }

    coreIndex ^= 0x55U;

    SYNC;

    if (bsp) MSG_("Done?%n");

    SYNC;

    SYNC;

    SYNC;

    SYNC;

    SYNC;

    SYNC;

    SYNC;

    SYNC;

    SYNC;
}

#endif
