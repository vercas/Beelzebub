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
#include <new>

#define MAX_PROCESSES 4095
#define MAX_THREADS 65536
//  TODO: Make these a setting.

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Utils;

IdPool<Process> ProcessIds;
IdPool<Thread> ThreadIds;

void Beelzebub::InitializeExecutionData()
{
    void * procList = new uintptr_t[MAX_PROCESSES];
    void * threadList = new uintptr_t[MAX_THREADS];

    ASSERT(procList != nullptr);
    ASSERT(threadList != nullptr);

    new (&ProcessIds) IdPool<Process>(procList, MAX_PROCESSES);
    new (&ThreadIds) IdPool<Thread>(threadList, MAX_THREADS);

    uintptr_t bootstrapProcessId = ProcessIds.Acquire(&BootstrapProcess);

    ASSERT(bootstrapProcessId == 0)(bootstrapProcessId);
    //  This one is discarded and reserved forever.

    bootstrapProcessId = ProcessIds.Acquire(&BootstrapProcess);

    ASSERT(BootstrapProcess.Id == bootstrapProcessId)(BootstrapProcess.Id)(bootstrapProcessId);
    //  Should be 1 now.
}

Result<SpawnProcessResult, Execution::Process *> Beelzebub::SpawnProcess()
{

}
