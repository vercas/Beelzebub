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

#ifdef __BEELZEBUB__TEST_STACKINT

#include "tests/stack_integrity.hpp"
#include <beel/sync/smp.lock.hpp>
#include "kernel.hpp"

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;

Barrier StackIntTestBarrier;

#define SYNC StackIntTestBarrier.Reach()

static constexpr size_t const TestCount = 300'000;

static constexpr uint32_t const TestSize = 2048; //  Should make 8 KiB of stack/thread-local/global space.

static constexpr uint32_t const HashStart = 2166136261;
static constexpr uint32_t const HashStep = 16777619;

uint32_t TestGlobalRegion[TestSize];
__thread uint32_t TestLocalRegion[TestSize];

static SmpLock GlobalRegionLock;

void TestStackIntegrity(bool bsp)
{
    uint32_t TestRegion[TestSize];
    uint32_t HashValue;
    uint32_t GlobalSeed = 1;

    if (bsp) Scheduling = false;

    SYNC;

    for (size_t test = TestCount; test > 0; --test)
    {
        HashValue = HashStart;

        for (int i = TestSize - 1; i >= 0; --i)
        {
            HashValue ^= GlobalSeed + i;
            HashValue *= HashStep;

            TestRegion[i] = HashValue;
        }

        HashValue = HashStart;

        for (int i = TestSize - 1; i >= 0; --i)
        {
            HashValue ^= GlobalSeed + i;
            HashValue *= HashStep;

            ASSERT(TestRegion[i] == HashValue
                , "Stack value corrupted! %X4 @ %Xp.%n"
                  "Expected %X4, got %X4."
                , i, &(TestRegion[i]), HashValue, TestRegion[i]);
        }

        GlobalSeed += TestSize;
        HashValue = HashStart;

        for (int i = TestSize - 1; i >= 0; --i)
        {
            HashValue ^= GlobalSeed + i;
            HashValue *= HashStep;

            TestLocalRegion[i] = HashValue;
        }

        HashValue = HashStart;

        for (int i = TestSize - 1; i >= 0; --i)
        {
            HashValue ^= GlobalSeed + i;
            HashValue *= HashStep;

            ASSERT(TestLocalRegion[i] == HashValue
                , "Thread-local value corrupted! %X4 @ %Xp.%n"
                  "Expected %X4, got %X4."
                , i, &(TestLocalRegion[i]), HashValue, TestLocalRegion[i]);
        }

        GlobalSeed += TestSize;
        HashValue = HashStart;

        GlobalRegionLock.Acquire();

        for (int i = TestSize - 1; i >= 0; --i)
        {
            HashValue ^= GlobalSeed + i;
            HashValue *= HashStep;

            TestGlobalRegion[i] = HashValue;
        }

        HashValue = HashStart;

        for (int i = TestSize - 1; i >= 0; --i)
        {
            HashValue ^= GlobalSeed + i;
            HashValue *= HashStep;

            ASSERT(TestGlobalRegion[i] == HashValue
                , "Global value corrupted! %X4 @ %Xp.%n"
                  "Expected %X4, got %X4."
                , i, &(TestGlobalRegion[i]), HashValue, TestGlobalRegion[i]);
        }

        GlobalRegionLock.Release();

        GlobalSeed += TestSize;
    }

    SYNC;

    if (bsp) Scheduling = true;
}

#endif
