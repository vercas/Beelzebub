/*
    Copyright (c) 2019 Alexandru-Mihai Maftei. All rights reserved.


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

#include "scheduler.hpp"
#include "timer.hpp"
#include "irqs.hpp"
#include "system/cpu.hpp"

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

static constexpr size_t const SizeShift = sizeof(size_t) == 4 ? 5 : 6;
static constexpr size_t const SizeMask = sizeof(size_t) * 8UL - 1UL;

static constexpr int const AffinityLevels = 3;
static constexpr size_t const PriorityBitmapSize = (Scheduler::PriorityLevels + sizeof(size_t) * 8 - 1) / (sizeof(size_t) * 8);

struct SchedulerData;
struct SchedulerQueue;

struct ThreadSchedulerState
{
    ThreadSchedulerState * Next, * Previous;
    SchedulerData * Scheduler;
    SchedulerQueue * Queue;
    SchedulerStatus Status;
    int Priority;
    Scheduler::AffinityMask Affinity;
};

DEFINE_THREAD_DATA(ThreadSchedulerState, SchedulingData)

struct SchedulerQueue
{
    ThreadSchedulerState * Pop(size_t const cpuIndex, bool & empty)
    {
        ThreadSchedulerState * cur = this->First;

        if unlikely(cur == nullptr)
            return nullptr;

        assert(cur->Scheduler == this->Scheduler)((void *)cur->Scheduler);
        assert(cur->Queue == this)((void *)cur->Queue);

        size_t const cpuIndexWord = (cpuIndex & ~SizeMask) >> SizeShift;
        size_t const cpuIndexBitMask = 1UL << (cpuIndex & SizeMask);

        if (0 != (cur->Affinity.Bitmap[cpuIndexWord] & cpuIndexBitMask))
        {
            this->First = cur->Next;

            if likely(cur->Next != nullptr)
            {
                cur->Next->Previous = cur->Previous;
                empty = false;
            }
            else
            {
                this->Last = nullptr;
                empty = true;
            }

            cur->Queue = nullptr;

            return cur;
        }

        empty = false;

        while ((cur = cur->Next) != nullptr)
        {
            assert(cur->Scheduler == this->Scheduler)((void *)cur->Scheduler);
            assert(cur->Queue == this)((void *)cur->Queue);

            if (0 != (cur->Affinity.Bitmap[cpuIndexWord] & cpuIndexBitMask))
            {
                cur->Previous->Next = cur->Next;

                if likely(cur->Next != nullptr)
                    cur->Next->Previous = cur->Previous;
                else
                    this->Last = cur->Previous;

                cur->Queue = nullptr;

                return cur;
            }
        }

        return nullptr;
    }

    void Pop(ThreadSchedulerState * cur, bool & empty)
    {
        assert(cur->Scheduler == this->Scheduler)((void *)cur->Scheduler);
        assert(cur->Queue == this)((void *)cur->Queue);

        if (cur->Next != nullptr)
            cur->Next->Previous = cur->Previous;
        else
            this->Last = cur->Previous;

        if (cur->Previous != nullptr)
            cur->Previous->Next = cur->Next;
        else
            this->First = cur->Next;

        empty = this->First == nullptr;

        cur->Queue = nullptr;
    }

    void Push(ThreadSchedulerState * tsc)
    {
        tsc->Previous = this->Last;
        //  tsc->Next is guaranteed to be null here.

        if likely(this->Last != nullptr)
            this->Last->Next = tsc;
        else
            this->First = tsc;

        this->Last = tsc;

        tsc->Queue = this;
    }

    ThreadSchedulerState * First, * Last;
    SchedulerData * Scheduler;
};

struct SchedulerData
{
    void Initialize()
    {
        this->Engaged = false;

        for (size_t i = 0; i < Scheduler::PriorityLevels; ++i)
            this->Queues[i].Scheduler = this;

        for (size_t i = 0; i < AffinityLevels; ++i)
            this->Next[i] = this;
    }

    ThreadSchedulerState * Pop(size_t cpuIndex)
    {
        ThreadSchedulerState * ret = nullptr;

        size_t bitmapIndex = PriorityBitmapSize;

        do
        {
            --bitmapIndex;

            if (this->Bitmap[bitmapIndex] != 0)
            {
                size_t highestPriority = SizeMask - __builtin_clzl(this->Bitmap[bitmapIndex]);
                bool empty;

                ret = this->Queues[bitmapIndex * sizeof(size_t) * 8 + highestPriority].Pop(cpuIndex, empty);

                if unlikely(empty)
                    this->Bitmap[bitmapIndex] &= ~(1UL << highestPriority);

                if (ret != nullptr)
                {
                    ret->Scheduler = nullptr;
                    ret->Next = ret->Previous = nullptr;
                }

                break;
            }
        } while (bitmapIndex > 0);

        return ret;
    }

    void Push(ThreadSchedulerState * tsc)
    {
        int priority = tsc->Priority;

        assert(priority >= 0 && priority < Scheduler::PriorityLevels)(priority);
        assert(tsc->Next == nullptr)((void *)tsc->Next);
        assert(tsc->Previous == nullptr)((void *)tsc->Previous);

        this->Queues[priority].Push(tsc);
        this->Bitmap[(priority & ~SizeMask) >> SizeShift] |= 1UL << (priority & SizeMask);

        tsc->Scheduler = this;
        tsc->Status = SchedulerStatus::Queued;
    }

    size_t CpuIndex;
    bool Engaged;
    ThreadSchedulerState * CurrentThread;
    ThreadSchedulerState * IdleThread;

    SchedulerData * Next[AffinityLevels];

    size_t Bitmap[PriorityBitmapSize];
    SchedulerQueue Queues[Scheduler::PriorityLevels];
};

struct WaitingList
{
    SchedulerData * Sched;

    ThreadSchedulerState * First, * Last;
};

namespace
{
    __thread SchedulerData MySchedulerData;

    ThreadSchedulerState * GetNext(SchedulerData * scdt)
    {
        SchedulerData * cur[AffinityLevels + 1];
        //  An extra level is used as a dummy value to know when all the real
        //  affinity levels have been exhausted.

        for (size_t i = 0; i <= AffinityLevels; ++i)
            cur[i] = scdt;

        do
        {
        loop_from_level_0:
            if (ThreadSchedulerState * ret = cur[0]->Pop(scdt->CpuIndex); ret != nullptr)
                return ret;

            for (int i = 0; i < AffinityLevels; ++i)
                if (SchedulerData * next = cur[i]->Next[i]; next != cur[i + 1])
                {
                    do cur[i] = next; while (i-- >= 0);

                    goto loop_from_level_0;
                }

            return scdt->IdleThread;
        } while (true);

        // FAIL("Unable to find a thread to schedule?!");
    }

    void SchedulerTick(SchedulerData * scdt)
    {
        InterruptContext * ic = Irqs::CurrentContext;
        bool enqueued = false;

        assert(ic->Next == nullptr, "An interrupt handler was pre-empted?!")((void *)ic->Next);

        if unlikely(Scheduler::Postpone)
        {
            enqueued = Timer::Enqueue(10usecs_l, &SchedulerTick, scdt);
            goto end_of_tick;
        }

        {   //  Limiting the scope of a couple of variables here.
            ThreadSchedulerState * const curThread = scdt->CurrentThread;
            scdt->Push(curThread);
            //  First put this thread back in the queue. It might need to be rescheduled again
            //  if it's got the highest priority and it's alone at that priority level.

            ThreadSchedulerState * const nextThread = GetNext(scdt);
            nextThread->Status = SchedulerStatus::Executing;

            SchedulingData.GetContainer(scdt->CurrentThread)->SwitchTo(SchedulingData.GetContainer(nextThread), ic->Registers);

            scdt->CurrentThread = nextThread;
        }

        enqueued = Timer::Enqueue(10msecs_l, &SchedulerTick, scdt);

    end_of_tick:
        assert(enqueued);
        (void)enqueued;

        return;
    }
}

/**********************
    Scheduler class
**********************/

bool Scheduler::Postpone = false;

static Scheduler::AffinityMask _AllCpusMask;
Scheduler::AffinityMask const & Scheduler::AllCpusMask = _AllCpusMask;

/*  Initialization  */

void Scheduler::Initialize(MainParameters * params)
{
    assert(Cpu::GetThread() == nullptr);

    InitializeIdleThread(params);

    if (params->BSP)
    {
        for (size_t i = 0; i < AffinityMask::Size; ++i)
            _AllCpusMask.Bitmap[i] = ~(size_t)0UL;

        assert(_AllCpusMask.IsAllOne());
        assert(AllCpusMask.IsAllOne());
    }

    MySchedulerData.Initialize();

    MySchedulerData.CurrentThread = &SchedulingData(Cpu::GetThread());
    MySchedulerData.IdleThread = MySchedulerData.CurrentThread;
    // assert(Timer::Enqueue(10msecs_l, &SchedulerTick, MySchedulerData));

    MySchedulerData.CurrentThread->Affinity.SetBit(MySchedulerData.CpuIndex);
    //  Only allow this thread to run on this core.

    Cpu::GetThread()->AcquireReference();
}

void Scheduler::Engage()
{
    SchedulerData * scdt = &MySchedulerData;

    bool const enqueued = Timer::Enqueue(10msecs_l, &SchedulerTick, scdt);

    assert(enqueued);

    msg_("My scheduler data: %Xp + %Xs%n", scdt, SizeOf<SchedulerData>);
}

void Scheduler::Enroll(Thread * thread)
{
    ThreadSchedulerState * tsc = &SchedulingData(thread);

    assert(tsc->Status == SchedulerStatus::Unscheduled || tsc->Status == SchedulerStatus::Blocked)("status", tsc->Status);

    if (tsc->Status == SchedulerStatus::Unscheduled)
        thread->AcquireReference();
    //  While threads are in the scheduling system, they cannot be deallocated.

    if likely(tsc->Affinity.IsAllZero())
        tsc->Affinity = _AllCpusMask;

    MySchedulerData.Push(tsc);
}

/*  Properties  */

SchedulerStatus Scheduler::GetStatus(Thread * thread)
{
    return SchedulingData(thread).Status;
}
