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
#include <exceptions.hpp>

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

static volatile bool Barrier;

static __startup void * TestThreadCode(void *);

__startup void TestDereferenceFailure(uintptr_t volatile * const testPtr)
{
    // DEBUG_TERM_ << " <POKING " << (void *)testPtr << ">";

    __try
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

        uintptr_t thisShouldFail = *testPtr = *(reinterpret_cast<uintptr_t *>(&TestThreadCode));
        //  This gets the first instruction of the `TestThreadCode` function and
        //  tries to write it to the target pointer.

#pragma GCC diagnostic pop

        ASSERT(false, "(( Test value that should've failed: %Xp ))%n", thisShouldFail);
    }
    __catch (x)
    {
        ASSERT_EQ("%up", ExceptionType::MemoryAccessViolation, x->Type);
        ASSERT_EQ("%Xp", testPtr, x->MemoryAccessViolation.Address);
        ASSERT_EQ("%XP", nullpaddr, x->MemoryAccessViolation.PhysicalAddress);
        ASSERT_EQ("%X2", MemoryLocationFlags::None, x->MemoryAccessViolation.PageFlags);
        ASSERT_EQ("%X1", MemoryAccessType::Write, x->MemoryAccessViolation.AccessType);
    }
}

void TestVas()
{
    Barrier = true;

    new (&testProcess) Process();

    Vmm::Initialize(&testProcess);

    new (&testThread) Thread(&testProcess);

    Handle res;

    //  Firstly, the kernel stack page of the test thread.

    uintptr_t stackVaddr = nullvaddr;

    res = Vmm::AllocatePages(CpuDataSetUp ? Cpu::GetProcess() : &BootstrapProcess
        , 3
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap | MemoryAllocationOptions::ThreadStack
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::Generic, stackVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate stack for VAS test thread: %H."
        , res);

    testThread.KernelStackTop = stackVaddr + 3 * PageSize;
    testThread.KernelStackBottom = stackVaddr;

    testThread.EntryPoint = &TestThreadCode;

    InitializeThreadState(&testThread);

    withInterrupts (false)
        BootstrapThread.IntroduceNext(&testThread);

    while (Barrier) CpuInstructions::DoNothing();
}

void * TestThreadCode(void *)
{
    uintptr_t vaddr = nullvaddr, vaddr1, vaddr2;

    Handle res = Vmm::AllocatePages(Cpu::GetProcess()
        , 4
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualUser
        , MemoryFlags::Userland | MemoryFlags::Writable
        , MemoryContent::Generic
        , vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate data for VAS test thread: %H."
        , res);

    // DEBUG_TERM_ << "Been given 4 anonymous user pages @ " << (void *)vaddr << "." << EndLine;
    vaddr1 = vaddr;

    memset((void *)vaddr, 0xCD, 4 * PageSize);

    // Debug::DebugTerminal->WriteHexTable(vaddr, 4 * PageSize, 32, false);

    withInterrupts (false)
    {
        TestDereferenceFailure(reinterpret_cast<uintptr_t volatile *>(vaddr - sizeof(uintptr_t)));

        if (vaddr + 4 * PageSize < Vmm::UserlandEnd)
            TestDereferenceFailure(reinterpret_cast<uintptr_t volatile *>(vaddr + 4 * PageSize));

        // DEBUG_TERM_ << EndLine;
    }

    vaddr = Vmm::UserlandStart + 1337 * PageSize;

    res = Vmm::AllocatePages(Cpu::GetProcess()
        , 5
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualUser
        , MemoryFlags::Userland | MemoryFlags::Writable
        , MemoryContent::Generic
        , vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate data for VAS test thread: %H."
        , res);

    ASSERT_EQ("%Xp", Vmm::UserlandStart + 1337 * PageSize, vaddr);
    vaddr2 = vaddr;

    // DEBUG_TERM_ << "Been given 5 anonymous user pages @ " << (void *)vaddr << "." << EndLine;

    memset((void *)vaddr, 0xAB, 5 * PageSize);

    // Debug::DebugTerminal->WriteHexTable(vaddr, 5 * PageSize, 32, false);

    withInterrupts (false)
    {
        if (vaddr - sizeof(uintptr_t) < vaddr1 || vaddr - sizeof(uintptr_t) >= vaddr1 + 4 * PageSize)
            TestDereferenceFailure(reinterpret_cast<uintptr_t volatile *>(vaddr - sizeof(uintptr_t)));

        if (vaddr + 5 * PageSize < Vmm::UserlandEnd && (vaddr + 5 * PageSize < vaddr1 || vaddr + PageSize >= vaddr1))
            TestDereferenceFailure(reinterpret_cast<uintptr_t volatile *>(vaddr + 5 * PageSize));

        // DEBUG_TERM_ << EndLine;
    }

    vaddr = nullvaddr;

    res = Vmm::AllocatePages(Cpu::GetProcess()
        , 6
        , MemoryAllocationOptions::AllocateOnDemand | MemoryAllocationOptions::VirtualUser | MemoryAllocationOptions::GuardLow
        , MemoryFlags::Userland | MemoryFlags::Writable
        , MemoryContent::Generic
        , vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate data for VAS test thread: %H."
        , res);

    // DEBUG_TERM_ << "Been given 6 anonymous user pages @ " << (void *)vaddr << "." << EndLine;
    // vaddr3 = vaddr;

    memset((void *)vaddr, 0x65, 6 * PageSize);

    // Debug::DebugTerminal->WriteHexTable(vaddr, 6 * PageSize, 32, false);

    withInterrupts (false)
    {
        if ((vaddr - sizeof(uintptr_t) < vaddr1 || vaddr - sizeof(uintptr_t) >= vaddr1 + 4 * PageSize)
         && (vaddr - sizeof(uintptr_t) < vaddr2 || vaddr - sizeof(uintptr_t) >= vaddr2 + 5 * PageSize))
            TestDereferenceFailure(reinterpret_cast<uintptr_t volatile *>(vaddr - sizeof(uintptr_t)));

        if (vaddr + 6 * PageSize < Vmm::UserlandEnd
        && (vaddr + 6 * PageSize < vaddr1 || vaddr + 2 * PageSize >= vaddr1)
        && (vaddr + 6 * PageSize < vaddr2 || vaddr +     PageSize >= vaddr2))
            TestDereferenceFailure(reinterpret_cast<uintptr_t volatile *>(vaddr + 5 * PageSize));

        // DEBUG_TERM_ << EndLine;
    }

    Barrier = false;

    while (true) CpuInstructions::Halt();
}

#endif
