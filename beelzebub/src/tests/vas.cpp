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
#include <beel/exceptions.hpp>

#include <kernel.hpp>

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

        FAIL("(( Test value that should've failed: %Xp ))%n", thisShouldFail);
    }
    __catch (x)
    {
        ASSERT_EQ("%up", ExceptionType::MemoryAccessViolation, x->Type);
        ASSERT_EQ("%Xp", testPtr, x->MemoryAccessViolation.Address.Pointer);
        ASSERT_EQ("%XP", nullpaddr, x->MemoryAccessViolation.PhysicalAddress);
        ASSERT_EQ("%X2", MemoryLocationFlags::None, x->MemoryAccessViolation.PageFlags);
        ASSERT_EQ("%X1", MemoryAccessType::Write, x->MemoryAccessViolation.AccessType);
    }
}

__startup void TestDereferenceFailure(vaddr_t const testPtr)
{
    TestDereferenceFailure((uintptr_t volatile *)(testPtr.Pointer));
}

void TestVas()
{
    Barrier = true;

    new (&testProcess) Process();

    Vmm::Initialize(&testProcess);

    new (&testThread) Thread(&testProcess);

    Handle res;

    //  Firstly, the kernel stack page of the test thread.

    vaddr_t stackVaddr = nullvaddr;

    res = Vmm::AllocatePages(nullptr
        , 3 * PageSize
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::ThreadStack, stackVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate stack for VAS test thread: %H."
        , res);

    testThread.KernelStackTop = (stackVaddr + 3 * PageSize).Value;
    testThread.KernelStackBottom = stackVaddr.Value;

    testThread.EntryPoint = &TestThreadCode;

    InitializeThreadState(&testThread);

    withInterrupts (false)
        BootstrapThread.IntroduceNext(&testThread);

    while (Barrier) CpuInstructions::DoNothing();
}

void * TestThreadCode(void *)
{
    vaddr_t vaddr = nullvaddr, vaddr1, vaddr2;

    Handle res = Vmm::AllocatePages(nullptr
        , 4 * PageSize
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualUser
        , MemoryFlags::Userland | MemoryFlags::Writable
        , MemoryContent::Generic
        , vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate data for VAS test thread: %H."
        , res);

    vaddr1 = vaddr;

    memset((void *)vaddr, 0xCD, 4 * PageSize);

    TestDereferenceFailure(vaddr - SizeOf<uintptr_t>);

    if (vaddr + 4 * PageSize < Vmm::UserlandEnd)
        TestDereferenceFailure(vaddr + 4 * PageSize);

    vaddr = Vmm::UserlandStart + 1337 * PageSize;

    res = Vmm::AllocatePages(nullptr
        , 5 * PageSize
        , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualUser
        , MemoryFlags::Userland | MemoryFlags::Writable
        , MemoryContent::Generic
        , vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate data for VAS test thread: %H."
        , res);

    ASSERT_EQ("%Xp", Vmm::UserlandStart + 1337 * PageSize, vaddr);
    vaddr2 = vaddr;

    memset((void *)vaddr, 0xAB, 5 * PageSize);

    if (vaddr - SizeOf<uintptr_t> < vaddr1 || vaddr - SizeOf<uintptr_t> >= vaddr1 + 4 * PageSize)
        TestDereferenceFailure(vaddr - SizeOf<uintptr_t>);

    if (vaddr + 5 * PageSize < Vmm::UserlandEnd && (vaddr + 5 * PageSize < vaddr1 || vaddr + PageSize >= vaddr1))
        TestDereferenceFailure(vaddr + 5 * PageSize);

    vaddr = nullvaddr;

    res = Vmm::AllocatePages(nullptr
        , 6 * PageSize
        , MemoryAllocationOptions::AllocateOnDemand | MemoryAllocationOptions::VirtualUser | MemoryAllocationOptions::GuardLow
        , MemoryFlags::Userland | MemoryFlags::Writable
        , MemoryContent::Generic
        , vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate data for VAS test thread: %H."
        , res);

    memset((void *)vaddr, 0x65, 6 * PageSize);

    if ((vaddr - SizeOf<uintptr_t> < vaddr1 || vaddr - SizeOf<uintptr_t> >= vaddr1 + 4 * PageSize)
     && (vaddr - SizeOf<uintptr_t> < vaddr2 || vaddr - SizeOf<uintptr_t> >= vaddr2 + 5 * PageSize))
        TestDereferenceFailure(vaddr - SizeOf<uintptr_t>);

    if (vaddr + 6 * PageSize < Vmm::UserlandEnd
    && (vaddr + 6 * PageSize < vaddr1 || vaddr + 2 * PageSize >= vaddr1)
    && (vaddr + 6 * PageSize < vaddr2 || vaddr +     PageSize >= vaddr2))
        TestDereferenceFailure(vaddr + 5 * PageSize);

    Barrier = false;

    while (true) CpuInstructions::Halt();
}

#endif
