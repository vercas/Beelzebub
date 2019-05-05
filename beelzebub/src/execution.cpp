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

#include "execution.hpp"
#include "kernel.hpp"
#include <memory/object_allocator_smp.hpp>
#include <memory/object_allocator_pools_heap.hpp>
#include <beel/utils/id.pool.hpp>

#define MAX_PROCESSES 4095
#define MAX_THREADS 65536
//  TODO: Make these a setting.

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Utils;

__section(bootstrap_process) Process Beelzebub::BootstrapProcess(1);
__section(bootstrap_thread)  Thread  Beelzebub::BootstrapThread(1, &BootstrapProcess);

IdPool<Process> ProcessIds;
IdPool<Thread> ThreadIds;

DEFINE_THREAD_DATA(int, TestThreadData1)
DEFINE_THREAD_DATA(int, TestThreadData2)
DEFINE_THREAD_DATA(int, TestThreadData3)
DEFINE_THREAD_DATA(int, TestThreadData4)
DEFINE_THREAD_DATA(int, TestThreadData5)

void Beelzebub::InitializeExecutionData()
{
    void * procList = new uintptr_t[MAX_PROCESSES];
    void * threadList = new uintptr_t[MAX_THREADS];

    ASSERT(procList != nullptr);
    ASSERT(threadList != nullptr);

    new (&ProcessIds) IdPool<Process>(procList, MAX_PROCESSES);
    new (&ThreadIds) IdPool<Thread>(threadList, MAX_THREADS);

    uintptr_t bootstrapProcessId = ProcessIds.Acquire(&BootstrapProcess);
    uintptr_t bootstrapThreadId = ThreadIds.Acquire(&BootstrapThread);

    ASSERT(bootstrapProcessId == 0)(bootstrapProcessId);
    ASSERT(bootstrapThreadId == 0)(bootstrapThreadId);
    //  These ones is discarded and reserved forever.

    bootstrapProcessId = ProcessIds.Acquire(&BootstrapProcess);
    bootstrapThreadId = ThreadIds.Acquire(&BootstrapThread);

    ASSERT(BootstrapProcess.Id == bootstrapProcessId)(BootstrapProcess.Id)(bootstrapProcessId);
    ASSERT(BootstrapThread.Id == bootstrapThreadId)(BootstrapThread.Id)(bootstrapThreadId);
    //  Should be 1 now.

    ASSERT(ProcessIds.Resolve(bootstrapProcessId) == &BootstrapProcess);
    ASSERT(ThreadIds.Resolve(bootstrapThreadId) == &BootstrapThread);

    ASSERT(ProcessIds.Resolve(2) == nullptr);
    ASSERT(ThreadIds.Resolve(2) == nullptr);
}

UniquePointer<Process> Beelzebub::SpawnProcess()
{
    uintptr_t id = ProcessIds.Acquire();

    if unlikely(id == IdPool<Process>::NoNext)
        return nullptr;

    assert(ProcessIds.Resolve(id) == nullptr);

    Process * obj = new Process((uint16_t)id);

    if unlikely(obj == nullptr)
    {
        ASSERTX(ProcessIds.Release(id))(id)XEND;

        return nullptr;
    }

    ASSERTX(ProcessIds.SetPointer(id, obj))
        (id)
        ((void *)obj)
        (reinterpret_cast<size_t>(obj) & ~IdPool<Process>::ValueMask)
        (ProcessIds.GetCapacity())
    XEND;

    return obj;
}

UniquePointer<Thread> Beelzebub::SpawnThread(Process * owner)
{
    uintptr_t id = ThreadIds.Acquire();

    if unlikely(id == IdPool<Thread>::NoNext)
        return nullptr;

    assert(ThreadIds.Resolve(id) == nullptr);

    Thread * obj = new Thread((uint16_t)id, owner);

    if unlikely(obj == nullptr)
    {
        ASSERTX(ThreadIds.Release(id))(id)XEND;

        return nullptr;
    }

    ASSERTX(ThreadIds.SetPointer(id, obj))
        (id)
        ((void *)obj)
        (reinterpret_cast<size_t>(obj) & ~IdPool<Thread>::ValueMask)
        (ThreadIds.GetCapacity())
    XEND;

    return obj;
}
