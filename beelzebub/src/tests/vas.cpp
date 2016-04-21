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

#ifdef __BEELZEBUB__TEST_VAS

#include <tests/vas.hpp>
#include <memory/vmm.hpp>
#include <execution/thread.hpp>
#include <execution/thread_init.hpp>
#include <execution/ring_3.hpp>

#include <kernel.hpp>
#include <entry.h>
#include <system/cpu.hpp>

#include <string.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

static Thread testThread;
static Process testProcess;

static void * TestThreadCode(void *);

void TestVas()
{
    new (&testProcess) Process();

    Vmm::Initialize(&testProcess);

    new (&testThread) Thread(&testProcess);

    Handle res;

    //  Firstly, the kernel stack page of the test thread.

    uintptr_t stackVaddr = nullvaddr;

    res = Vmm::AllocatePages(CpuDataSetUp ? Cpu::GetProcess() : &BootstrapProcess
        , 3, MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap | MemoryAllocationOptions::ThreadStack
        , MemoryFlags::Global | MemoryFlags::Writable, stackVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate stack for VAS test thread: %H."
        , res);

    testThread.KernelStackTop = stackVaddr + 3 * PageSize;
    testThread.KernelStackBottom = stackVaddr;

    testThread.EntryPoint = &TestThreadCode;

    InitializeThreadState(&testThread);

    withInterrupts (false)
        BootstrapThread.IntroduceNext(&testThread);

}

void * TestThreadCode(void *)
{
    uintptr_t vaddr = nullvaddr;

    Handle res = Vmm::AllocatePages(Cpu::GetProcess()
        , 4, MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualUser
        , MemoryFlags::Userland | MemoryFlags::Writable, vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate data for VAS test thread: %H."
        , res);

    DEBUG_TERM_ << "Been given 4 anonymous user pages @ " << (void *)vaddr << "." << EndLine;

    memset((void *)vaddr, 0xCD, 4 * PageSize);

    Debug::DebugTerminal->WriteHexTable(vaddr, 4 * PageSize, 16, false);

    vaddr = Vmm::UserlandStart + 1337 * PageSize;

    res = Vmm::AllocatePages(Cpu::GetProcess()
        , 5, MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualUser
        , MemoryFlags::Userland | MemoryFlags::Writable, vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate data for VAS test thread: %H."
        , res);

    ASSERT_EQ("%Xp", Vmm::UserlandStart + 1337 * PageSize, vaddr);

    DEBUG_TERM_ << "Been given 5 anonymous user pages @ " << (void *)vaddr << "." << EndLine;

    memset((void *)vaddr, 0xAB, 5 * PageSize);

    Debug::DebugTerminal->WriteHexTable(vaddr, 5 * PageSize, 16, false);

    vaddr = nullvaddr;

    res = Vmm::AllocatePages(Cpu::GetProcess()
        , 6, MemoryAllocationOptions::AllocateOnDemand | MemoryAllocationOptions::VirtualUser
        , MemoryFlags::Userland | MemoryFlags::Writable, vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate data for VAS test thread: %H."
        , res);

    DEBUG_TERM_ << "Been given 6 anonymous user pages @ " << (void *)vaddr << "." << EndLine;

    memset((void *)vaddr, 0xF0, 6 * PageSize);

    Debug::DebugTerminal->WriteHexTable(vaddr, 6 * PageSize, 16, false);

    while (true) CpuInstructions::Halt();
}

#endif
