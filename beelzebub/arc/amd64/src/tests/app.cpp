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

#ifdef __BEELZEBUB__TEST_APP

#include <tests/app.hpp>
#include <initrd.hpp>
#include <execution/runtime64.hpp>
#include <execution/thread.hpp>
#include <execution/thread_init.hpp>
#include <execution/ring_3.hpp>
#include <memory/vmm.hpp>

#include <kernel.hpp>
#include <entry.h>
#include <system/cpu.hpp>

#include <string.h>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

static uintptr_t rtlib_base = 0x1000000;

static Thread testThread;
static Thread testWatcher;
static Process testProcess;

static uintptr_t loadtestStart = nullvaddr, loadtestEnd = nullvaddr;
static uintptr_t const userStackPageCount = 254;

static __cold void * JumpToRing3(void *);
static __cold void * WatchTestThread(void *);

__startup Handle HandleLoadtest(uintptr_t const vaddr, size_t const size)
{
    //  First make it accessible to userland.
    //  It doesn't matter if other parts of the initrd are affected.
    //  TODO: Yet?

    vaddr_t const firstPage = RoundDown(vaddr, PageSize);
    vaddr_t const lastPage = RoundUp(vaddr + size, PageSize);

    for (size_t offset = 0; firstPage + offset < lastPage; offset += PageSize)
    {
        Handle res = Vmm::SetPageFlags(&BootstrapProcess, firstPage + offset
            , MemoryFlags::Global | MemoryFlags::Userland);
        //  Modules are normally global-supervisor-writable. This one needs to
        //  be global-userland-readable.

        ASSERT(res.IsOkayResult()
            , "Failed to change flags of page at %Xp for loadtest app: %H."
            , firstPage + offset, res);
    }

    // DEBUG_TERM << EndLine
    //     << "############################## LOADTEST APP START ##############################"
    //     << EndLine;

    // ElfHeader1 * eh1 = reinterpret_cast<ElfHeader1 *>(addr);

    // DEBUG_TERM << *eh1 << EndLine;

    // ASSERT(eh1->Identification.Class == ElfClass::Elf64);
    // ASSERT(eh1->Identification.DataEncoding == ElfDataEncoding::LittleEndian);

    // ElfHeader2_64 * eh2 = reinterpret_cast<ElfHeader2_64 *>(addr + sizeof(ElfHeader1));

    // DEBUG_TERM << Hexadecimal << *eh2 << Decimal << EndLine;

    // // entryPoint = eh2->EntryPoint;

    // ElfHeader3 * eh3 = reinterpret_cast<ElfHeader3 *>(addr + sizeof(ElfHeader1) + sizeof(ElfHeader2_64));

    // DEBUG_TERM << *eh3 << EndLine;

    // ASSERT(eh3->SectionHeaderTableEntrySize == sizeof(ElfSectionHeader_64)
    //     , "Unusual section header type: %us; expected %us."
    //     , (size_t)(eh3->SectionHeaderTableEntrySize), sizeof(ElfSectionHeader_64));

    // DEBUG_TERM << EndLine
    //     << "############################### LOADTEST APP END ###############################"
    //     << EndLine;

    loadtestStart = vaddr;
    loadtestEnd = vaddr + size;

    return HandleResult::Okay;
}

Spinlock<> TestRegionLock;

void TestApplication()
{
    //  First get the loadtest app's location.

    ASSERT(InitRd::Loaded);

    Handle file = InitRd::FindItem("/apps/loadtest.exe");

    ASSERT(file.IsType(HandleType::InitRdFile)
        , "Failed to find loadtest app in InitRD: %H.", file);

    FileBoundaries bnd = InitRd::GetFileBoundaries(file);

    ASSERT(bnd.Start != 0 && bnd.Size != 0);

    //  Then attempt parsing it.

    Handle res = HandleLoadtest(bnd.Start, bnd.Size);

    ASSERT(res.IsOkayResult(), "Error in handling loadtest app: %H.", res);

    //  Then prepare the necessary processes and threads.

    TestRegionLock.Reset();
    TestRegionLock.Acquire();

    new (&testProcess) Process();

    Vmm::Initialize(&testProcess);

    new (&testThread) Thread(&testProcess);
    new (&testWatcher) Thread(&testProcess);

    //  Then the test thread.

    uintptr_t stackVaddr = nullvaddr;

    res = Vmm::AllocatePages(nullptr, 3
        , MemoryAllocationOptions::Commit   | MemoryAllocationOptions::VirtualKernelHeap
        | MemoryAllocationOptions::GuardLow | MemoryAllocationOptions::GuardHigh
        | MemoryAllocationOptions::ThreadStack
        , MemoryFlags::Global | MemoryFlags::Writable, stackVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate stack for test userland thread: %H."
        , res);

    testThread.KernelStackTop = stackVaddr + 3 * PageSize;
    testThread.KernelStackBottom = stackVaddr;

    testThread.EntryPoint = &JumpToRing3;

    InitializeThreadState(&testThread);
    //  This sets up the thread so it goes directly to the entry point when switched to.

    withInterrupts (false)
        BootstrapThread.IntroduceNext(&testThread);

    //  Then the watcher thread.

    stackVaddr = nullvaddr;

    res = Vmm::AllocatePages(nullptr, 3
        , MemoryAllocationOptions::Commit   | MemoryAllocationOptions::VirtualKernelHeap
        | MemoryAllocationOptions::GuardLow | MemoryAllocationOptions::GuardHigh
        | MemoryAllocationOptions::ThreadStack
        , MemoryFlags::Global | MemoryFlags::Writable, stackVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate stack for test watcher thread: %H."
        , res);

    testWatcher.KernelStackTop = stackVaddr + 3 * PageSize;
    testWatcher.KernelStackBottom = stackVaddr;

    testWatcher.EntryPoint = &WatchTestThread;

    InitializeThreadState(&testWatcher);
    //  This sets up the thread so it goes directly to the entry point when switched to.

    withInterrupts (false)
        testThread.IntroduceNext(&testWatcher);

    //  That's all, folks. The other threads finish the work.
}

void * JumpToRing3(void * arg)
{
    Handle res;

    //  First, the userland stack page.

    uintptr_t userStackBottom = nullvaddr;

    res = Vmm::AllocatePages(&testProcess
        , userStackPageCount
        , MemoryAllocationOptions::AllocateOnDemand | MemoryAllocationOptions::VirtualUser
        | MemoryAllocationOptions::GuardLow         | MemoryAllocationOptions::GuardHigh
        , MemoryFlags::Userland | MemoryFlags::Writable
        , userStackBottom);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate userland stack for app test thread: %H."
        , res);

    uintptr_t userStackTop = userStackBottom + userStackPageCount * PageSize;

    //  Then, deploy the runtime.

    StartupData * stdat = nullptr;

    res = Runtime64::Deploy(rtlib_base, stdat);

    ASSERT(res.IsOkayResult()
        , "Failed to deploy runtime64 library into test app process: %H."
        , res);

    ASSERT(stdat != nullptr);

    //  Then give the runtime relevant data.

    stdat->MemoryImageStart = loadtestStart;
    stdat->MemoryImageEnd = loadtestEnd;

    //  Finally, a region for test incrementation.

    vaddr_t testRegVaddr = 0x300000000000;

    res = Vmm::AllocatePages(&testProcess
        , 0x30000 / PageSize
        , MemoryAllocationOptions::AllocateOnDemand | MemoryAllocationOptions::VirtualUser
        , MemoryFlags::Userland | MemoryFlags::Writable
        , testRegVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate app test region in userland: %H.%n"
          "Stack is between %Xp and %Xp."
        , res, userStackBottom, userStackTop);

    TestRegionLock.Release();

    //  And finish by going to ring 3.

    DEBUG_TERM_ << "About to go to ring 3!" << EndLine;

    CpuInstructions::InvalidateTlb(reinterpret_cast<void const *>(rtlib_base + Runtime64::Template.GetEntryPoint()));

    return GoToRing3_64(rtlib_base + Runtime64::Template.GetEntryPoint(), userStackTop);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

void * WatchTestThread(void *)
{
    TestRegionLock.Spin();

    uint8_t  const * const data  = reinterpret_cast<uint8_t  const *>(0x300000000008);
    uint64_t const * const data2 = reinterpret_cast<uint64_t const *>(0x300000000000);

    while (true)
    {
        void * activeThread = Cpu::GetThread();

        // MSG("WATCHER (%Xp) sees %u1 & %u8!%n", activeThread, *data, *data2);
        // DEBUG_TERM  << "WATCHER (" << Hexadecimal << activeThread << Decimal
        //             << ") sees " << *data << " & " << *data2 << "!" << EndLine;

        CpuInstructions::Halt();
    }
}

#pragma GCC diagnostic pop

#endif
