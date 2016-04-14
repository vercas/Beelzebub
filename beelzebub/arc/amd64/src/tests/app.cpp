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
#include <_print/paging.hpp>

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
static uintptr_t const userStackBottom = 0x40000000;
static uintptr_t const userStackTop = 0x40000000 + PageSize;

static Elf rtElf;

static __cold void * JumpToRing3(void *);
static __cold void * WatchTestThread(void *);

Handle HandleLoadtest(size_t const index
                    , jg_info_module_t const * const module
                    , vaddr_t const vaddr
                    , size_t const size)
{
    uint8_t * const addr = reinterpret_cast<uint8_t *>(vaddr);
    // size_t const len = module->length;

    // executable = addr;

    DEBUG_TERM << EndLine
        << "############################## LOADTEST APP START ##############################"
        << EndLine;

    ElfHeader1 * eh1 = reinterpret_cast<ElfHeader1 *>(addr);

    DEBUG_TERM << *eh1 << EndLine;

    ASSERT(eh1->Identification.Class == ElfClass::Elf64);
    ASSERT(eh1->Identification.DataEncoding == ElfDataEncoding::LittleEndian);

    ElfHeader2_64 * eh2 = reinterpret_cast<ElfHeader2_64 *>(addr + sizeof(ElfHeader1));

    DEBUG_TERM << Hexadecimal << *eh2 << Decimal << EndLine;

    // entryPoint = eh2->EntryPoint;

    ElfHeader3 * eh3 = reinterpret_cast<ElfHeader3 *>(addr + sizeof(ElfHeader1) + sizeof(ElfHeader2_64));

    DEBUG_TERM << *eh3 << EndLine;

    ASSERT(eh3->SectionHeaderTableEntrySize == sizeof(ElfSectionHeader_64)
        , "Unusual section header type: %us; expected %us."
        , (size_t)(eh3->SectionHeaderTableEntrySize), sizeof(ElfSectionHeader_64));

    DEBUG_TERM << EndLine
        << "############################### LOADTEST APP END ###############################"
        << EndLine;

    return HandleResult::Okay;
}

Spinlock<> TestRegionLock;

void TestApplication()
{
    TestRegionLock.Reset();
    TestRegionLock.Acquire();

    new (&testProcess) Process();
    //  Initialize a new process for thread series B.

    Vmm::Initialize(&testProcess);

    new (&testThread) Thread(&testProcess);
    new (&testWatcher) Thread(&testProcess);

    Handle res;

    //  Firstly, the kernel stack page of the test thread.

    uintptr_t stackVaddr;

    res = Vmm::AllocatePages(CpuDataSetUp ? Cpu::GetProcess() : &BootstrapProcess
        , 3, MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
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

    //  Secondly, the kernel stack page of the watcher thread.

    res = Vmm::AllocatePages(CpuDataSetUp ? Cpu::GetProcess() : &BootstrapProcess
        , 3, MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
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
}

__cold uint8_t * AllocateTestRegion()
{
    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    vaddr_t const vaddr1 = 0x30000;
    size_t const size = vaddr1;
    vaddr_t const vaddr2 = Vmm::KernelHeapCursor.FetchAdd(size);
    //  Test pages.

    for (size_t offset = 0; offset < size; offset += PageSize)
    {
        paddr_t const paddr = Cpu::GetData()->DomainDescriptor->PhysicalAllocator->AllocatePage(desc);
        
        ASSERT(paddr != nullpaddr && desc != nullptr
            , "Unable to allocate a physical page for test page of process %Xp!"
            , &testProcess);

        res = Vmm::MapPage(&testProcess, vaddr1 + offset, paddr
            , MemoryFlags::Userland | MemoryFlags::Writable, desc);

        ASSERT(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP) as test page in owning process: %H."
            , vaddr1 + offset, paddr
            , res);

        res = Vmm::MapPage(&testProcess, vaddr2 + offset, paddr
            , MemoryFlags::Global | MemoryFlags::Writable, desc);

        ASSERT(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP) as test page with boostrap memory manager: %H."
            , vaddr2 + offset, paddr
            , res);
    }

    TestRegionLock.Release();

    return (uint8_t *)(uintptr_t)vaddr2;
}

void * JumpToRing3(void * arg)
{
    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    //  First, the userland stack page.

    paddr_t const stackPaddr = Cpu::GetData()->DomainDescriptor->PhysicalAllocator->AllocatePage(desc);

    ASSERT(stackPaddr != nullpaddr && desc != nullptr
        , "Unable to allocate a physical page for test thread user stack!");

    res = Vmm::MapPage(&testProcess, userStackBottom, stackPaddr
        , MemoryFlags::Userland | MemoryFlags::Writable, desc);

    ASSERT(res.IsOkayResult()
        , "Failed to map page at %Xp (%XP) for test thread user stack: %H."
        , userStackBottom, stackPaddr
        , res);

    //  Then, deploy the runtime.

    res = Runtime64::Deploy(rtlib_base);

    ASSERT(res.IsOkayResult()
        , "Failed to deploy runtime64 library into test app process: %H."
        , res);

    //  Finally, a region for test incrementation.

    AllocateTestRegion();

    DEBUG_TERM_ << "About to go to ring 3!" << EndLine;

    //while (true) CpuInstructions::Halt();

    CpuInstructions::InvalidateTlb(reinterpret_cast<void const *>(rtlib_base + Runtime64::Template.GetEntryPoint()));

    return GoToRing3_64(rtlib_base + Runtime64::Template.GetEntryPoint(), userStackTop);
}

void * WatchTestThread(void *)
{
    TestRegionLock.Spin();

    uint8_t  const * const data  = reinterpret_cast<uint8_t  const *>(0x30008);
    uint64_t const * const data2 = reinterpret_cast<uint64_t const *>(0x30000);

    while (true)
    {
        void * activeThread = Cpu::GetThread();

        // MSG("WATCHER (%Xp) sees %u1 & %u8!%n", activeThread, *data, *data2);
        // DEBUG_TERM  << "WATCHER (" << Hexadecimal << activeThread << Decimal
        //             << ") sees " << *data << " & " << *data2 << "!" << EndLine;
    }
}

#endif
