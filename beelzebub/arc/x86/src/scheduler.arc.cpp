/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

/*  Note that the implementation of this header is architecture-specific.  */

#include "scheduler.hpp"
#include "memory/vmm.hpp"
#include <math.h>
#include <new>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;

static size_t MaxProcesses = 4095;
static size_t MaxThreads = sizeof(tid_t);
static size_t MaxScheduledThreads = 100;

static Process * * Processes;
static Thread * * Threads;

static __thread Thread * * MyThreads;
static __thread Thread * IdleThread;
static __thread Thread * ActiveThread;

/**********************
    Scheduler class
**********************/

/*  Initialization  */

Handle Scheduler::Initialize(bool bsp)
{
    if (bsp)
    {
        //  TODO: Read parameters for limits.

        vsize_t size;
        vaddr_t vaddr;

        if (MaxProcesses * SizeOf<Process *> >= 2 * PageSize)
        {
            size = RoundUp(MaxProcesses * SizeOf<Process *>, PageSize);
            vaddr = nullvaddr;

            Handle res = Vmm::AllocatePages(nullptr
                , size
                , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
                | MemoryAllocationOptions::GuardFull
                , MemoryFlags::Global | MemoryFlags::Writable
                , MemoryContent::ProcessList
                , vaddr);

            if (res != HandleResult::Okay)
                return HandleResult::OutOfMemory;

            Processes = (Process * *)(vaddr.Pointer);
        }
        else
        {
            Processes = new (std::nothrow) Process*[MaxProcesses];

            if (Processes == nullptr)
                return HandleResult::OutOfMemory;
        }

        if (MaxThreads * SizeOf<Thread *> >= 2 * PageSize)
        {
            size = RoundUp(MaxThreads * SizeOf<Thread *>, PageSize);
            vaddr = nullvaddr;

            Handle res = Vmm::AllocatePages(nullptr
                , size
                , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
                | MemoryAllocationOptions::GuardFull
                , MemoryFlags::Global | MemoryFlags::Writable
                , MemoryContent::ThreadList
                , vaddr);

            if (res != HandleResult::Okay)
                return HandleResult::OutOfMemory;

            Threads = (Thread * *)(vaddr.Pointer);
        }
        else
        {
            Threads = new (std::nothrow) Thread*[MaxThreads];

            if (Threads == nullptr)
                return HandleResult::OutOfMemory;
        }
    }

    MyThreads = new (std::nothrow) Thread*[MaxScheduledThreads];

    if (MyThreads == nullptr)
        return HandleResult::OutOfMemory;

    return HandleResult::Okay;
}

void Scheduler::Engage()
{

}

/*  Properties  */

size_t Scheduler::GetMaximumProcesses()
{
    return MaxProcesses;
}

size_t Scheduler::GetMaximumThreads()
{
    return MaxThreads;
}

size_t Scheduler::GetMaximumScheduledThreads()
{
    return MaxScheduledThreads;
}
