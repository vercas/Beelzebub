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
#include <execution/elf.hpp>
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

uint8_t * executable;
ElfProgramHeader_64 * segments;
size_t segmentCount;
uintptr_t entryPoint;

Thread testThread;
Thread testWatcher;
Process testProcess;
uintptr_t const userStackBottom = 0x40000000;
uintptr_t const userStackTop = 0x40000000 + PageSize;

__cold void * JumpToRing3(void *);
__cold void * WatchTestThread(void *);

Handle HandleLoadtest(size_t const index
                    , jg_info_module_t const * const module
                    , const vaddr_t vaddr
                    , size_t const size)
{
    uint8_t * const addr = reinterpret_cast<uint8_t *>(vaddr);
    size_t const len = module->length;

    executable = addr;

    msg("%n");

    ElfHeader1 * eh1 = reinterpret_cast<ElfHeader1 *>(addr);

    *(Debug::DebugTerminal) << *eh1 << EndLine;

    ASSERT(eh1->Identification.Class == ElfClass::Elf64
        , "Expected class ELF64!");
    ASSERT(eh1->Identification.DataEncoding == ElfDataEncoding::LittleEndian
        , "Expected class ELF64!");

    ElfHeader2_64 * eh2 = reinterpret_cast<ElfHeader2_64 *>(addr + sizeof(ElfHeader1));

    *(Debug::DebugTerminal) << Hexadecimal << *eh2 << Decimal << EndLine;

    entryPoint = eh2->EntryPoint;

    ElfHeader3 * eh3 = reinterpret_cast<ElfHeader3 *>(addr + sizeof(ElfHeader1) + sizeof(ElfHeader2_64));

    *(Debug::DebugTerminal) << *eh3 << EndLine;

    ASSERT(eh3->SectionHeaderTableEntrySize == sizeof(ElfSectionHeader_64)
        , "Unusual section header type: %us; expected %us."
        , (size_t)(eh3->SectionHeaderTableEntrySize), sizeof(ElfSectionHeader_64));

    // msg("%nSections:%n");

    // ElfSectionHeader_64 * sectionCursor = reinterpret_cast<ElfSectionHeader_64 *>(
    //     addr + eh2->SectionHeaderTableOffset
    // );

    // char const * sectionNames = reinterpret_cast<char const *>(addr + sectionCursor[eh3->SectionNameStringTableIndex].Offset);

    // for (size_t i = eh3->SectionHeaderTableEntryCount, j = 1; i > 0; --i, ++j, ++sectionCursor)
    // {
    //     msg("\t#%us:%n"
    //         "\t\tName: %X4 (%s)%n"
    //         "\t\tType: %u8 (%s)%n"
    //         "\t\tFlags:       %X8%n"
    //         "\t\tAddress:     %X8%n"
    //         "\t\tOffset:      %X8%n"
    //         "\t\tSize:        %X8%n"
    //         "\t\tLink:        %X4%n"
    //         "\t\tInfo:        %X4%n"
    //         "\t\tAlignment:   %X8%n"
    //         "\t\tEntrry Size: %X8%n"
    //         , j
    //         , sectionCursor->Name, sectionNames + sectionCursor->Name
    //         , (uint64_t)(sectionCursor->Type), EnumToString(sectionCursor->Type)
    //         , sectionCursor->Flags
    //         , sectionCursor->Address
    //         , sectionCursor->Offset
    //         , sectionCursor->Size
    //         , sectionCursor->Link
    //         , sectionCursor->Info
    //         , sectionCursor->AddressAlignment
    //         , sectionCursor->EntrySize
    //     );
    // }

    msg("%nSegments:%n");

    ElfProgramHeader_64 * programCursor = reinterpret_cast<ElfProgramHeader_64 *>(
        addr + eh2->ProgramHeaderTableOffset
    );

    segments = programCursor;
    segmentCount = eh3->ProgramHeaderTableEntryCount;

    for (size_t i = eh3->ProgramHeaderTableEntryCount, j = 1; i > 0; --i, ++j, ++programCursor)
    {
        // msg("\t#%us:%n"
        //     "\t\tType: %u8 (%s)%n"
        //     "\t\tFlags:     %X4%n"
        //     "\t\tOffset:    %X8%n"
        //     "\t\tVAddr:     %X8%n"
        //     "\t\tPAddr:     %X8%n"
        //     "\t\tVSize:     %X8%n"
        //     "\t\tPSize:     %X8%n"
        //     "\t\tAlignment: %X8%n"
        //     , j
        //     , (uint64_t)(programCursor->Type), EnumToString(programCursor->Type)
        //     , programCursor->Flags
        //     , programCursor->Offset
        //     , programCursor->VAddr
        //     , programCursor->PAddr
        //     , programCursor->PSize
        //     , programCursor->VSize
        //     , programCursor->Alignment
        // );

        *(Debug::DebugTerminal) << "#" << j << ": " << *programCursor << EndLine;

        if (programCursor->Type == ElfProgramHeaderType::Dynamic)
        {
            msg("\tEntries:%n");

            ElfDynamicEntry * dynEntCursor = reinterpret_cast<ElfDynamicEntry *>(
                addr + programCursor->Offset
            );
            size_t offset = 0, k = 0;

            do
            {
                msg("\t\t\t#%us:%n"
                    "\t\t\t\tTag:   %Xp (%s)%n"
                    "\t\t\t\tValue: %Xp%n"
                    , k
                    , (intptr_t)(dynEntCursor->Tag), EnumToString(dynEntCursor->Tag)
                    , dynEntCursor->Value
                );

                ++dynEntCursor;
                offset += sizeof(ElfDynamicEntry);
                ++k;
            } while (offset < programCursor->PSize && dynEntCursor->Tag != ElfDynamicEntryTag::Null);
        }
    }

    msg("%n");

    return HandleResult::Okay;
}

Spinlock<> TestRegionLock;

void TestApplication()
{
    TestRegionLock.Acquire();

    new (&testProcess) Process();
    //  Initialize a new process for thread series B.

    Vmm::Initialize(&testProcess);

    new (&testThread) Thread(&testProcess);
    new (&testWatcher) Thread(&testProcess);

    Handle res;

    //  Firstly, the kernel stack page of the test thread.

    uintptr_t stackVaddr;

    res = Vmm::AllocatePages(CpuDataSetUp ? Cpu::GetData()->ActiveThread->Owner : &BootstrapProcess
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

    res = Vmm::AllocatePages(CpuDataSetUp ? Cpu::GetData()->ActiveThread->Owner : &BootstrapProcess
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

    //  ... then, the userland stack page.

    paddr_t const stackPaddr = Cpu::GetData()->DomainDescriptor->PhysicalAllocator->AllocatePage(desc);

    ASSERT(stackPaddr != nullpaddr && desc != nullptr
        , "Unable to allocate a physical page for test thread user stack!");

    res = Vmm::MapPage(&testProcess, userStackBottom, stackPaddr
        , MemoryFlags::Userland | MemoryFlags::Writable, desc);

    ASSERT(res.IsOkayResult()
        , "Failed to map page at %Xp (%XP) for test thread user stack: %H."
        , userStackBottom, stackPaddr
        , res);

    //  Then, the app's segments.

    ElfProgramHeader_64 * programCursor = segments;

    for (size_t i = segmentCount, j = 1; i > 0; --i, ++j, ++programCursor)
    {
        if (programCursor->Type != ElfProgramHeaderType::Load)
            continue;

        vaddr_t const segVaddr    = RoundDown(programCursor->VAddr, PageSize);
        vaddr_t const segVaddrEnd = RoundUp  (programCursor->VAddr + programCursor->VSize, PageSize);

        MemoryFlags pageFlags = MemoryFlags::Userland;

        if (0 != (programCursor->Flags & ElfProgramHeaderFlags::Writable))
            pageFlags |= MemoryFlags::Writable;
        if (0 != (programCursor->Flags & ElfProgramHeaderFlags::Executable))
            pageFlags |= MemoryFlags::Executable;

        for (vaddr_t vaddr = segVaddr; vaddr < segVaddrEnd; vaddr += PageSize)
        {
            paddr_t const paddr = Cpu::GetData()->DomainDescriptor->PhysicalAllocator->AllocatePage(desc);

            ASSERT(paddr != nullpaddr && desc != nullptr
                , "Unable to allocate a physical page for test app segment #%us!"
                , j);

            res = Vmm::MapPage(&testProcess, vaddr, paddr, pageFlags, desc);

            ASSERT(res.IsOkayResult()
                , "Failed to map page at %Xp (%XP) for test app segment #%us: %H."
                , vaddr, paddr
                , j, res);
        }

        memcpy(reinterpret_cast<void *>(programCursor->VAddr)
            , executable + programCursor->Offset, programCursor->PSize);

        if (programCursor->VSize > programCursor->PSize)
            memset(reinterpret_cast<void *>(programCursor->VAddr + programCursor->PSize)
                , 0, programCursor->VSize - programCursor->PSize);
    }

    //  Finally, a region for test incrementation.

    AllocateTestRegion();

    MSG_("About to go to ring 3!%n");

    //while (true) CpuInstructions::Halt();

    CpuInstructions::InvalidateTlb(reinterpret_cast<void const *>(entryPoint));

    return GoToRing3_64(entryPoint, userStackTop);
}

void * WatchTestThread(void *)
{
    TestRegionLock.Spin();

    uint8_t  const * const data  = reinterpret_cast<uint8_t  const *>(0x30008);
    uint64_t const * const data2 = reinterpret_cast<uint64_t const *>(0x30000);

    while (true)
    {
        Thread * activeThread = Cpu::GetData()->ActiveThread;

        MSG("WATCHER (%Xp) sees %u1 & %u8!%n", activeThread, *data, *data2);
    }
}

#endif
