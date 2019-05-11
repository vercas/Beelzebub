/*
    Copyright (c) 2017 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, Vmm::FreePages of charge, to any person obtaining a copy
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

#if defined(__BEELZEBUB__TEST_VMM)

#include "tests/vmm.hpp"
#include "memory/vmm.hpp"
#include "cores.hpp"
#include "scheduler.hpp"
#include "watchdog.hpp"
#include "system/debug.registers.hpp"
#include <new>

#include <beel/sync/smp.lock.hpp>
#include <math.h>
#include <debug.hpp>

#define PRINT
// #define CROSS_INTEGRITY_TEST_ENABLE

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

Barrier VmmTestBarrier;
// static Barrier VmmCrossTestBarrier;

#define SYNC VmmTestBarrier.Reach()
// #define CROSS_SYNC VmmCrossTestBarrier.Reach()

static SmpLock DeleteLock {};

static constexpr size_t const RandomIterations = 10'000;
static constexpr size_t const CacheSize = 2048;
static Atomic<void const *> Cache[CacheSize];
static constexpr size_t const SyncerCount = 10;
static Atomic<void const *> Syncers[SyncerCount];
static __thread vaddr_t MyCache[CacheSize];
#ifdef PRINT
static Atomic<size_t> RandomerCounter {0};
#endif
//  There is no space on the stack for this one.

#ifdef CROSS_INTEGRITY_TEST_ENABLE
// static Atomic<int> CrossEntry;
static Atomic<uint32_t *> CrossTestRegion;

static __solid void TestVmmCrossIntegrity(bool const bsp);
#endif

static __solid void TestVmmIntegrity(bool const bsp);
// static __hot void DumpStack(INTERRUPT_HANDLER_ARGS, void * address, System::BreakpointProperties & bp);

void TestVmm(bool const bsp)
{
    if (bsp) Scheduler::Postpone = true;

    size_t coreIndex = Cpu::GetData()->Index;

    auto getPtr = [](bool commit = true)
    {
        vaddr_t testptr = nullvaddr;

        Handle res = Vmm::AllocatePages(nullptr
            , vsize_t(0x4000)
            , commit
                ? (MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap)
                : MemoryAllocationOptions::VirtualKernelHeap
            , MemoryFlags::Global | MemoryFlags::Writable
            , MemoryContent::Generic
            , testptr);

        ASSERTX(res == HandleResult::Okay)(res)XEND;
        ASSERTX(testptr >= Vmm::KernelStart
            , "Returned address (%Xp) is not in the kernel heap..?", testptr)XEND;

        return testptr;
    };

    auto delPtr = [](vaddr_t vaddr)
    {
        Handle res = Vmm::FreePages(nullptr, vaddr, vsize_t(0x4000));

        ASSERTX(res == HandleResult::Okay)(res)XEND;
    };

    // SYNC;

    // if (bsp)
    // {
    //     System::DebugRegisters::AddBreakpoint(&(Vmm::KVas.Tree.Root)
    //         , 8, true, System::BreakpointCondition::DataWrite, &DumpStack);

    //     // System::DebugRegisters::AddBreakpoint(&(Vmm::KVas.First)
    //     //     , 8, true, System::BreakpointCondition::DataWrite, &DumpStack);
    // }

    SYNC;

#ifdef PRINT
    if (bsp) MSG_("Filling array.%n");
#endif

    SYNC;

#ifdef PRINT
    uint64_t perfStart = 0, perfEnd = 0;

    SYNC;

    perfStart = CpuInstructions::Rdtsc();
#endif

    vaddr_t cur = getPtr(), dummy = getPtr();

    for (size_t i = 0; i < CacheSize; ++i)
    {
        void const * expected = nullptr;

        if ((Cache + i)->CmpXchgStrong(expected, cur.Pointer))
            cur = getPtr();
    }

#ifdef PRINT
    perfEnd = CpuInstructions::Rdtsc();

    SYNC;

    MSG_("Core %us did filling in %us cycles: %us per slot.%n"
        , coreIndex, perfEnd - perfStart
        , (perfEnd - perfStart + CacheSize / 2 + 1) / (CacheSize + 2));

    SYNC;

    if (bsp) MSG_("First check.%n");
#endif

    for (size_t i = 0; i < CacheSize; ++i)
        ASSERT(cur.Pointer != Cache[i]);

    SYNC;

#ifdef PRINT
    if (bsp) MSG_("Full diff check.%n");

    SYNC;
#endif

    for (size_t i = coreIndex; i < CacheSize; i += Cores::GetCount())
        for (size_t j = i + 1; j < CacheSize; ++j)
            ASSERT(Cache[i] != Cache[j]);

    SYNC;

    withLock (DeleteLock)
    {
#ifdef PRINT
        MSG_("Core %us frees %Xp.%n", coreIndex, cur);
#endif

        delPtr(cur);
    }

    cur = nullvaddr;

    SYNC;

#ifdef PRINT
    if (bsp) MSG_("Individual stability 1.%n");

    SYNC;

    perfStart = CpuInstructions::Rdtsc();
#endif

    for (size_t i = 0; i < RandomIterations; ++i)
        delPtr(getPtr());

#ifdef PRINT
    perfEnd = CpuInstructions::Rdtsc();

    MSG_("Core %us did %us Vmm::AllocatePages & Vmm::FreePages pairs in %us cycles; %us cycles per pair.%n"
        , coreIndex, RandomIterations, perfEnd - perfStart, (perfEnd - perfStart + RandomIterations / 2) / RandomIterations);

    SYNC;

    if (bsp) MSG_("Individual stability 2.%n");
#endif

    for (size_t i = 0; i < CacheSize; ++i)
        MyCache[i] = nullvaddr;

    SYNC;

#ifdef PRINT
    perfStart = CpuInstructions::Rdtsc();
#endif

    for (size_t i = 0, j = 0; j < RandomIterations; ++j)
    {
        if (MyCache[i] == nullvaddr)
            MyCache[i] = getPtr();
        else
        {
            delPtr(MyCache[i]);
            MyCache[i] = nullvaddr;
        }

        if (++i == CacheSize) i = 0;
    }

    for (size_t i = 0; i < CacheSize; ++i)
        if (MyCache[i] != nullvaddr)
            delPtr(MyCache[i]);

#ifdef PRINT
    perfEnd = CpuInstructions::Rdtsc();

    MSG_("Core %us did %us Vmm::AllocatePages & Vmm::FreePages in the same order in %us cycles; %us cycles per operation.%n"
        , coreIndex, RandomIterations, perfEnd - perfStart, (perfEnd - perfStart + RandomIterations / 2) / RandomIterations);
#endif

    SYNC;

#ifdef PRINT
    if (bsp) MSG_("Randomer 1!%n");

    SYNC;
#endif

    // if (Watchdog::AmIInCharge()) for (;;) { }

    coreIndex ^= 0x55U;

#ifdef PRINT
    perfStart = CpuInstructions::Rdtsc();
#endif

    for (size_t i = 0, j = 0; j < RandomIterations; ++j)
    {
        // MSG_("@%us:%us:%us@", coreIndex ^ 0x55U, i, j);

    retry:
        void const * old = nullptr;

        if (Cache[i] != nullptr)
        {
            old = (Cache + i)->Xchg(old);

            if unlikely(old == nullptr)
                goto retry;
            //  If it became null in the meantime, retry.

            delPtr(vaddr_t(old));
        }
        else
        {
            if (cur == nullvaddr)
                cur = getPtr();

            if likely((Cache + i)->CmpXchgStrong(old, cur.Pointer))
                cur = nullvaddr;
            else
                goto retry;
            //  If it became non-null in the meantime, retry.
        }

        i += coreIndex;

        while (i >= CacheSize)
            i -= CacheSize;
    }

#ifdef PRINT
    perfEnd = CpuInstructions::Rdtsc();
#endif

    coreIndex ^= 0x55U;

    SYNC;

#ifdef PRINT
    RandomerCounter += perfEnd - perfStart;

    SYNC;

    if (bsp)
    {
        size_t const itcnt = Cores::GetCount() * RandomIterations;

        MSG_("%us cores did %us random Vmm::AllocatePages/Vmm::FreePages under random congestion in %us cycles; %us cycles per operation.%n"
            , Cores::GetCount(), itcnt, RandomerCounter.Load(), (RandomerCounter + itcnt / 2) / itcnt);
    }

    SYNC;
#endif

    if (bsp)
        for (size_t i = 0; i < CacheSize; ++i)
            if (Cache[i] == nullptr)
                Cache[i] = getPtr().Pointer;

    if (cur != nullvaddr)
        delPtr(cur);

    SYNC;

#ifdef PRINT
    if (bsp)
    {
        MSG_("Randomer 2!%n");

        RandomerCounter.Store(0);
    }

    SYNC;
#endif

    coreIndex ^= 0x55U;

#ifdef PRINT
    perfStart = CpuInstructions::Rdtsc();
#endif

    for (size_t i = 0, j = 0; j < RandomIterations; ++j)
    {
        cur = getPtr();
        vaddr_t old { (Cache + i)->Xchg(cur.Pointer) };

        delPtr(old);

        i += coreIndex;

        while (i >= CacheSize)
            i -= CacheSize;
    }

#ifdef PRINT
    perfEnd = CpuInstructions::Rdtsc();
#endif

    coreIndex ^= 0x55U;

    SYNC;

#ifdef PRINT
    RandomerCounter += perfEnd - perfStart;

    SYNC;

    if (bsp)
    {
        size_t const itcnt = Cores::GetCount() * RandomIterations;

        MSG_("%us cores did %us random Vmm::AllocatePages/Vmm::FreePages under random congestion in %us cycles; %us cycles per operation.%n"
            , Cores::GetCount(), itcnt, RandomerCounter.Load(), (RandomerCounter + itcnt / 2) / itcnt);
    }

    SYNC;

    if (bsp) MSG_("Cleanup.%n");

    SYNC;
#endif

    if (bsp)
        for (size_t i = 0; i < CacheSize; ++i)
            if (Cache[i] != nullptr)
                delPtr(vaddr_t(Cache[i]));

    delPtr(dummy);

#ifdef CROSS_INTEGRITY_TEST_ENABLE
    SYNC;

    if (Cores::GetCount() > 1)
    {
        if (bsp)
        {
            // VmmCrossTestBarrier.Reset(2);

            // CrossEntry.Store(0);

    #ifdef PRINT
            MSG_("Cross integrity test.%n");
    #endif
        }

        SYNC;

        // int blergh = 0;

        if (bsp)
            TestVmmCrossIntegrity(true);
        else //if (CrossEntry.CmpXchgStrong(blergh, 1))
            TestVmmCrossIntegrity(false);

        //  Yes, exactly two cores will execute that function.
    }
    #ifdef PRINT
    else
        MSG_("Skipping VMM cross integrity test.%n");
    #endif
#endif

    SYNC;

    if (bsp)
    {
        // DEBUG_TERM_ << &(Memory::Vmm::KVas);

        MSG_("VMM integrity test.%n");

        // ::PrintMemoryOps = true;
    }

    SYNC;

    TestVmmIntegrity(bsp);

    SYNC;

#ifdef PRINT
    if (bsp)
    {
        MSG_("Done with VMM test.%n");

        // ::PrintMemoryOps = false;
    }

    SYNC;
#endif

    if (bsp)
    {
        Scheduler::Postpone = false;

        // System::DebugRegisters::RemoveBreakpoint(&(Vmm::KVas.Tree.Root));
        // // System::DebugRegisters::RemoveBreakpoint(&(Vmm::KVas.First));
    }
}

#include "memory/pmm.hpp"

static constexpr size_t const TestCount = 1'000, CrossTestCount = 200, IterationCount = 1'000;

static constexpr uint32_t const TestSize = 2048; //  Should make 8 KiB of stack/thread-local/global space.

static constexpr uint32_t const HashStart = 2166136251;
static constexpr uint32_t const HashStep = 16707613;

static __thread uint32_t ExpectedTop;

#ifdef CROSS_INTEGRITY_TEST_ENABLE
void TestVmmCrossIntegrity(bool const bsp)
{
    // MSG_("Stack pointer in TestVmmIntegrity is %Xp.%n", GetCurrentStackPointer());

    auto getPtr = []()
    {
        vaddr_t testptr = nullvaddr;

        Handle res = Vmm::AllocatePages(nullptr
            , RoundUp(vsize_t(TestSize * sizeof(uint32_t)), PageSize)
            , MemoryAllocationOptions::AllocateOnDemand | MemoryAllocationOptions::VirtualKernelHeap
            , MemoryFlags::Global | MemoryFlags::Writable
            , MemoryContent::Generic
            , testptr);

        ASSERTX(res == HandleResult::Okay)(res)XEND;
        ASSERTX(testptr >= Vmm::KernelStart
            , "Returned address (%Xp) is not in the kernel heap..?", testptr)XEND;

        // System::DebugRegisters::AddBreakpoint(reinterpret_cast<uint32_t *>(testptr) + TestSize - 1
        //     , 4, true, System::BreakpointCondition::DataWrite, &DumpStack);

        return reinterpret_cast<uint32_t *>(const_cast<void *>(testptr.Pointer));
    };

    auto delPtr = [](uint32_t * vaddr)
    {
        Handle res = Vmm::FreePages(nullptr, vaddr_t(vaddr)
            , RoundUp(vsize_t(TestSize * sizeof(uint32_t)), PageSize));

        ASSERTX(res == HandleResult::Okay)(res)XEND;

        // System::DebugRegisters::RemoveBreakpoint(vaddr + TestSize - 1);
    };

    if (bsp)
        CrossTestRegion.Store(nullptr);

    // CROSS_SYNC;
    SYNC;

    uint32_t HashValue, OldHashValue;
    uint32_t GlobalSeed = 1;

// #ifdef PRINT
//     if (bsp)
//         MSG_("Hashing. KVas root is at offset %us.%n"
//             , reinterpret_cast<size_t>(&(Vmm::KVas.Tree.Root)) - reinterpret_cast<size_t>(&(Vmm::KVas)));

//     SYNC;
// #endif

    for (size_t test = CrossTestCount; test > 0; --test)
    {
        uint32_t * testRegion;

        if (bsp)
            for (size_t i = Cores::GetCount(); i > 1; --i)
            {
                CrossTestRegion.Store(getPtr());

                if (i > 2)
                    while (CrossTestRegion.Load() != nullptr)
                        DO_NOTHING();
                //  Meh.
            }
        else
            while ((testRegion = CrossTestRegion.Xchg(nullptr)) == nullptr)
                DO_NOTHING();

        // CROSS_SYNC;
        SYNC;

        if (!bsp)
            for (size_t iteration = IterationCount; iteration > 0; --iteration)
            {
                HashValue = HashStart;

                // MSG_("# Write #");

                for (int i = TestSize - 1; i >= 0; --i)
                {
                    HashValue ^= GlobalSeed + i;
                    HashValue *= HashStep;

                    // if unlikely(i == TestSize - 1)
                    //     ExpectedTop = HashValue;

                    testRegion[i] = HashValue;
                }

                OldHashValue = HashValue = HashStart;

                // MSG_("# Read #");

                for (int i = TestSize - 1; i >= 0; --i)
                {
                    HashValue ^= GlobalSeed + i;
                    HashValue *= HashStep;

                    ASSERTX((OldHashValue ^ (GlobalSeed + i)) * HashStep == HashValue)
                        (OldHashValue)(GlobalSeed)(i)(HashStep)(HashValue)
                        (GlobalSeed + i)(OldHashValue ^ (GlobalSeed + i))
                        ((OldHashValue ^ (GlobalSeed + i)) * HashStep)XEND;

                    if unlikely(testRegion[i] != HashValue)
                    {
                        paddr_t paddr;
                        FrameSize size;
                        uint32_t refCnt;

                        Handle res = Vmm::Translate(nullptr, reinterpret_cast<uintptr_t>(testRegion + i), paddr);

                        if unlikely(res != HandleResult::Okay)
                            MSG_("Failed to translate address of faulty slot %Xp: %H%n", testRegion + i, res);
                        else
                            res = Pmm::GetFrameInfo(paddr, size, refCnt);

                        ASSERTX(testRegion[i] == HashValue
                            , "AoD value corrupted! Expected %X4, got %X4."
                            , HashValue, testRegion[i])
                            (i)("address", (void *)&(testRegion[i]))(test)(iteration)
                            (paddr)(res)(size)(refCnt)XEND;
                    }

                    OldHashValue = HashValue;
                }

                GlobalSeed += TestSize;
            }

        // CROSS_SYNC;
        SYNC;

        // if (!bsp)
        //     delPtr(testRegion);

        if (bsp)
            for (size_t i = Cores::GetCount(); i > 1; --i)
            {
                while (CrossTestRegion.Load() == nullptr)
                    DO_NOTHING();

                delPtr(CrossTestRegion.Xchg(nullptr));
            }
        else
        {
            uint32_t * old;

            do old = nullptr; while (!CrossTestRegion.CmpXchgStrong(old, testRegion));
        }

        // CROSS_SYNC;
        SYNC;

        if (bsp && test % 20 == 0)
            MSG("%us... ", test);
    }
}
#endif

void TestVmmIntegrity(bool const bsp)
{
    // MSG_("Stack pointer in TestVmmIntegrity is %Xp.%n", GetCurrentStackPointer());

    auto getPtr = []()
    {
        vaddr_t testptr = nullvaddr;

        Handle res = Vmm::AllocatePages(nullptr
            , RoundUp(vsize_t(TestSize * sizeof(uint32_t)), PageSize)
            , MemoryAllocationOptions::AllocateOnDemand | MemoryAllocationOptions::VirtualKernelHeap
            , MemoryFlags::Global | MemoryFlags::Writable
            , MemoryContent::Generic
            , testptr);

        ASSERTX(res == HandleResult::Okay)(res)XEND;
        ASSERTX(testptr >= Vmm::KernelStart
            , "Returned address (%Xp) is not in the kernel heap..?", testptr)XEND;

        // System::DebugRegisters::AddBreakpoint(reinterpret_cast<uint32_t *>(testptr) + TestSize - 1
        //     , 4, true, System::BreakpointCondition::DataWrite, &DumpStack);

        return reinterpret_cast<uint32_t volatile *>(const_cast<void volatile *>(testptr.Pointer));
    };

    auto delPtr = [](uint32_t volatile * vaddr)
    {
        Handle res = Vmm::FreePages(nullptr, vaddr_t((void const *)vaddr)
            , RoundUp(vsize_t(TestSize * sizeof(uint32_t)), PageSize));

        ASSERTX(res == HandleResult::Okay)(res)XEND;

        // System::DebugRegisters::RemoveBreakpoint(vaddr + TestSize - 1);
    };

    uint32_t HashValue, OldHashValue;
    uint32_t GlobalSeed = 1;

// #ifdef PRINT
//     if (bsp)
//         MSG_("Hashing. KVas root is at offset %us.%n"
//             , reinterpret_cast<size_t>(&(Vmm::KVas.Tree.Root)) - reinterpret_cast<size_t>(&(Vmm::KVas)));

//     SYNC;
// #endif

    for (size_t test = TestCount; test > 0; --test)
    {
        uint32_t volatile * testRegion = getPtr();
        paddr_t origPaddr = nullpaddr;

        for (size_t iteration = IterationCount; iteration > 0; --iteration)
        {
            HashValue = HashStart;
            // int j = 0;

            // MSG_("# Write #");

            for (int i = TestSize - 1; i >= 0; --i)
            {
                HashValue ^= GlobalSeed + i;
                HashValue *= HashStep;

                // if unlikely(i == TestSize - 1)
                //     ExpectedTop = HashValue;

                testRegion[i] = HashValue;

                if unlikely(i == TestSize - 1 && iteration == IterationCount)
                {
                    paddr_t paddr;
                    Handle res = Vmm::Translate(nullptr
                        , vaddr_t(const_cast<uint32_t *>(testRegion + i)), paddr);

                    if unlikely(res != HandleResult::Okay)
                        MSG_("Failed to translate address of first slot %Xp: %H; Value is %X4%n", testRegion + i, res, testRegion[i]);
                    else
                        origPaddr = paddr;
                }
            }

            // ASSERTX(j == 2048)(j)XEND;

            OldHashValue = HashValue = HashStart;

            // MSG_("# Read #");

            for (int i = TestSize - 1; i >= 0; --i)
            {
                HashValue ^= GlobalSeed + i;
                HashValue *= HashStep;

                ASSERTX((OldHashValue ^ (GlobalSeed + i)) * HashStep == HashValue)
                    (OldHashValue)(GlobalSeed)(i)(HashStep)(HashValue)
                    (GlobalSeed + i)(OldHashValue ^ (GlobalSeed + i))
                    ((OldHashValue ^ (GlobalSeed + i)) * HashStep)XEND;

                if unlikely(testRegion[i] != HashValue)
                {
                    paddr_t paddr;
                    FrameSize size;
                    uint32_t refCnt;

                    Handle res = Vmm::Translate(nullptr
                        , vaddr_t(const_cast<uint32_t *>(testRegion + i)), paddr);

                    ASSERTX(paddr == origPaddr)(paddr)(origPaddr)(i)(test)(iteration)XEND;

                    if unlikely(res != HandleResult::Okay)
                        MSG_("Failed to translate address of faulty slot %Xp: %H%n", testRegion + i, res);
                    else
                        res = Pmm::GetFrameInfo(paddr, size, refCnt);

                    ASSERTX(testRegion[i] == HashValue
                        , "AoD value corrupted! Expected %X4, got %X4."
                        , HashValue, testRegion[i])
                        (i)("address", (void *)&(testRegion[i]))(test)(iteration)
                        (paddr)(res)(size)(refCnt)XEND;
                }

                OldHashValue = HashValue;
            }

            GlobalSeed += TestSize;
        }

        delPtr(testRegion);

        // if (test % 100 == 0)
        //     MSG_("%us:%us... ", Cpu::GetData()->Index, test);
    }
}

#include "utils/stack_walk.hpp"
// #include "_print/isr.hpp"

// void DumpStack(INTERRUPT_HANDLER_ARGS, void * address, System::BreakpointProperties & bp)
// {
//     (void)bp;

//     uint32_t val = *reinterpret_cast<uint32_t *>(address);

//     if (val == ExpectedTop)
//     {
//         // MSG_("@ Value is as expected: %X4 @", val);

//         goto end;
//     }

//     withLock (Debug::MsgSpinlock)
//     {
//         DEBUG_TERM << "-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --" << Terminals::EndLine;

//         MSG("Breakpoint Address: %Xp%n", address);
//         MSG("Value at Address: %X4%n", val);
//         MSG("Expected: %X4%n", ExpectedTop);

//         DEBUG_TERM << state;

//         uintptr_t stackPtr = state->RSP;
//         uintptr_t const stackEnd = RoundUp(stackPtr, PageSize.Value);

//         if ((stackPtr & (sizeof(size_t) - 1)) != 0)
//         {
//             MSG("Stack pointer was not a multiple of %us! (%Xp)%n"
//                 , sizeof(size_t), stackPtr);

//             stackPtr &= ~((uintptr_t)(sizeof(size_t) - 1));
//         }

//         bool odd;
//         for (odd = false; stackPtr < stackEnd; stackPtr += sizeof(size_t), odd = !odd)
//         {
//             MSG("%X2|%Xp|%Xs|%s"
//                 , (uint16_t)(stackPtr - state->RSP)
//                 , stackPtr
//                 , *((size_t const *)stackPtr)
//                 , odd ? "\r\n" : "\t");
//         }

//         if (odd) MSG("%n");

//         Utils::StackFrame stackFrame;

//         if (stackFrame.LoadFirst(state->RSP, state->RBP, state->RIP))
//         {
//             do
//             {
//                 MSG("[Func %Xp; Stack top %Xp + %us]%n"
//                     , stackFrame.Function, stackFrame.Top, stackFrame.Size);

//             } while (stackFrame.LoadNext());
//         }

//         DEBUG_TERM << "-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --" << Terminals::EndLine;
//     }

// end:
//     END_OF_INTERRUPT();
// }

#endif
