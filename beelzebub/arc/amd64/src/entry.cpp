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

#include "system/cpuid.hpp"
#include "execution/thread_init.hpp"
#include "terminals/vbe.hpp"

#include "memory/pmm.hpp"
#include "memory/pmm.arc.hpp"
#include "memory/vmm.hpp"
#include "memory/vmm.arc.hpp"
#include "memory/regions.hpp"

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    #include <memory/object_allocator_smp.hpp>
    #include <memory/object_allocator_pools_heap.hpp>
#endif

#include "keyboard.hpp"
#include "system/serial_ports.hpp"
#include "system/timers/pit.hpp"
#include "system/cpu.hpp"
#include "system/fpu.hpp"

#include "initrd.hpp"
#include "kernel.image.hpp"

#include "kernel.hpp"
#include "entry.h"
#include "global_options.hpp"
#include "cores.hpp"

#include <beel/sync/atomic.hpp>
#include <math.h>
#include <string.h>

#ifdef __BEELZEBUB__TEST_APP
#include "tests/app.hpp"
#endif

#include <debug.hpp>
#include "_print/registers.hpp"
#include "_print/gdt.hpp"

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;
using namespace Beelzebub::System::Timers;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Terminals;

/*******************
    ENTRY POINTS
*******************/

void kmain_bsp()
{
    return Beelzebub::Main();
}

#if   defined(__BEELZEBUB_SETTINGS_SMP)
void kmain_ap()
{
    return Beelzebub::Secondary();
}
#endif

/*****************************
    COMMAND-LINE ARGUMENTS
*****************************/

Handle Beelzebub::ParseKernelArguments()
{
    Handle res;
    char * cmdline = nullptr;

    /*  First obtain the argument string  */

    size_t const moduleCount = (size_t)JG_INFO_ROOT_EX->module_count;

    for (size_t i = 0; i < moduleCount; ++i)
    {
        jg_info_module_t const * const module = JG_INFO_MODULE_EX + i;

        if (strcmp("kernel64", JG_INFO_STRING_EX + module->name) == 0)
        {
            cmdline = JG_INFO_STRING_EX + module->cmdline;

            break;
        }
    }

    ASSERT(cmdline != nullptr
        , "Failed to find command-line arguments of the kernel module!");

    /*  Then initialize the global arguments  */

    res = InstanceGlobalOptions();

    ASSERT(res.IsOkayResult()
        , "Failed to initialize global list of command-line options: %H"
        , res);

    /*  Now prepare the parser  */

    CommandLineOptionParser parser;

    res = parser.StartParsing(cmdline);

    ASSERT(res.IsOkayResult()
        , "Failed to start parsing command-line options: %H"
        , res);

    CommandLineOptionBatchState batch;

    res = parser.StartBatch(batch, CommandLineOptionsHead);

    ASSERT(res.IsOkayResult()
        , "Failed to start batch for processing command-line options: %H"
        , res);

    /*  And do the deed  */

    size_t counter = 0, lastOff = batch.Offset;

    while (!(res = batch.Next()).IsResult(HandleResult::UnsupportedOperation))
    {
        ASSERT(res.IsOkayResult()
            , "Failed to parse command-line argument #%us: %H; %s"
            , counter, res, parser.InputString + lastOff);

        ++counter;
        lastOff = batch.Offset;
    }

    return HandleResult::Okay;
}

/**********************
    PHYSICAL MEMORY
**********************/

/**
 *  <summary>
 *  Sanitizes the memory map and initializes the page allocator over the local
 *  region's available entries.
 *  </summary>
 */
__startup Handle InitializePhysicalAllocator(jg_info_mmap_t * map // TODO: Don't depend on Jegudiel; let Jegudiel depend on Beelzebub!
                                           , size_t cnt
                                           , paddr_t freeStart
                                           , Domain * domain)
{
    Handle res; //  Used for intermediary results.

    //  First step is aligning the memory map.
    //  Also, yes, I could've used bits 'n powers of two here.
    //  But I'm hoping to future-proof the code a bit, in case
    //  I would ever target a platform whose page size is not
    //  A power of two.
    //  Moreover, this code doesn't need to be lightning-fast. :L

    paddr_t start = RoundUp(freeStart, PageSize), end = nullpaddr;
    jg_info_mmap_t * firstMap = nullptr, * lastMap = nullptr;

    for (size_t i = 0; i < cnt; i++)
    {
        jg_info_mmap_t * m = map + i;
        //  Current map.

        if (paddr_t(m->address + m->length) <= freeStart || !m->available)
            continue;

        if (firstMap == nullptr)
            firstMap = m;

        paddr_t mEnd { m->address + m->length };

        if (mEnd > end)
        {
            end = mEnd;
            lastMap = m;
        }
    }

    //  DUMPING MMAP ?

    onDebug if (domain == &Domain0)
    {
        msg("%nIND |A|    Address     |     Length     |       End      |%n");

        for (size_t i = 0; i < cnt; i++)
        {
            jg_info_mmap_t & e = map[i];
            //  Current map.

            //if (e.available == 0) continue;

            msg("%X2|%c|%X8|%X8|%X8|%n"
                , (uint16_t)i
                , e.available ? 'A' : ' '
                , e.address
                , e.length
                , e.address + e.length);
        }

        msg("Free memory start: %X8%n", freeStart);

        msg("Initializing memory over entries #%us-%us...%n", (size_t)(firstMap - map)
                                                            , (size_t)(lastMap - map));
        msg(" Address rage: %X8-%X8.%n", start, end);
        msg(" Maps start at %Xp.%n", firstMap);

        msg("%n");//*/
    }

    //  SPACE CREATION

    for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
        if (m->available != 0 && m->length >= (2 * PageSize.Value))
        {
            if (m->address < start.Value && (m->address + m->length) > start.Value)
                //  Means this entry crosses the start of free memory.
                //CreateAllocationSpace(start, m->address + m->length, domain);
                PmmArc::CreateAllocationSpace(start, paddr_t(m->address + m->length));
            else
                //CreateAllocationSpace(m->address, m->address + m->length, domain);
                PmmArc::CreateAllocationSpace(paddr_t(m->address), paddr_t(m->address + m->length));
        }

    //  PAGE RESERVATION

    for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
        if (m->available == 0)
        {
            res = Pmm::ReserveRange(paddr_t(m->address), psize_t(m->length));

            assert(res.IsOkayResult() || res.IsResult(HandleResult::PagesOutOfAllocatorRange)
                , "Failed to reserve page range %XP-%XP: %H."
                , m->address, m->address + m->length
                , res);
        }

    return HandleResult::Okay;
}

/**
 *  <summary>
 *  Initializes the system's physical memory for all domains.
 *  </summary>
 */
Handle Beelzebub::InitializePhysicalMemory()
{
    Handle res;

    res = InitializePhysicalAllocator(JG_INFO_MMAP_EX, JG_INFO_ROOT_EX->mmap_count, paddr_t(JG_INFO_ROOT_EX->free_paddr), &Domain0);

    ASSERT(res.IsOkayResult()
        , "Failed to initialize the physical memory allocator for domain 0: %H.%n"
        , res);

    return HandleResult::Okay;
}

/*********************
    VIRTUAL MEMORY
*********************/

__startup void RemapTerminal(TerminalBase * const terminal)
{
    Handle res;

    if (terminal->Capabilities->Type == TerminalType::PixelMatrix)
    {
        VbeTerminal * const term = (VbeTerminal *)terminal;
        vsize_t const size { RoundUp((size_t)term->Pitch * (size_t)term->Height, PageSize.Value) };
        //  Yes, the size is aligned with page boundaries.

        if (term->VideoMemory >= VmmArc::KernelHeapStart
         && term->VideoMemory <  VmmArc::KernelHeapEnd)
            return;
        //  Nothing to do, folks.
        
        vaddr_t vaddr = nullvaddr;

        res = Vmm::AllocatePages(&BootstrapProcess
            , size
            , MemoryAllocationOptions::Reserve | MemoryAllocationOptions::VirtualKernelHeap
            , MemoryFlags::Global | MemoryFlags::Writable
            , MemoryContent::VbeFramebuffer
            , vaddr);

        ASSERT(res.IsOkayResult()
            , "Failed to reserve %Xs bytes to map VBE framebuffer: %H."
            , size, res);

        res = Vmm::MapRange(&BootstrapProcess
            , vaddr, paddr_t(term->VideoMemory), size
            , MemoryFlags::Global | MemoryFlags::Writable
            , MemoryMapOptions::NoReferenceCounting);

        ASSERT(res.IsOkayResult()
            , "Failed to map range at %Xp to %XP (%Xs bytes) for VBE framebuffer: %H."
            , vaddr, term->VideoMemory, size, res);

        term->VideoMemory = vaddr.Value;
    }

    //  TODO: Make a VGA text terminal and also handle it here.
}

/**
 *  <summary>
 *  Prepares the virtual address space to be used as intended by the kernel.
 *  </summary>
 */
Handle Beelzebub::InitializeVirtualMemory()
{
    Handle res;
    //  Used for intermediary results.

    //  PAGING INITIALIZATION

    paddr_t const pml4_paddr = Pmm::AllocateFrame(1, AddressMagnitude::_32bit);

    ASSERT(pml4_paddr != nullpaddr, "Failed to allocate frame for System process's PML4.");

    ::memset((void *)(pml4_paddr.Value), 0, PageSize.Value);
    //  Clear it all out!

    new (&BootstrapProcess) Process(1);
    BootstrapProcess.SetPagingTable(pml4_paddr);
    BootstrapProcess.SetName("System");

    VmmArc::Page1GB = BootstrapCpuid.CheckFeature(CpuFeature::Page1GB);
    VmmArc::NX      = BootstrapCpuid.CheckFeature(CpuFeature::NX     );

    Vmm::Bootstrap(&BootstrapProcess);
    ++BootstrapProcess.ActiveCoreCount;

    RemapTerminal(MainTerminal);

    //  Now mapping the lower 16 MiB.

    res = Vmm::MapRange(&BootstrapProcess
        , vaddr_t(VmmArc::IsaDmaStart), nullpaddr, vsize_t(VmmArc::IsaDmaLength)
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryMapOptions::NoReferenceCounting);

    ASSERT(res.IsOkayResult()
        , "Failed to map range at %Xp (%XP) for ISA DMA: %H."
        , VmmArc::IsaDmaStart, nullpaddr
        , res);

    //  TODO: Management for ISA DMA.

    Cpu::SetCr0(Cpu::GetCr0().SetWriteProtect(true));

    BootstrapProcess.SetActive();

    return HandleResult::Okay;
}

/**************
    MODULES
**************/

/**
 *  <summary>
 *  Processes a module.
 *  </summary>
 */
__startup Handle HandleModule(size_t const index, jg_info_module_t const * const module)
{
    Handle res;

    vsize_t const size = RoundUp(vsize_t(module->length), PageSize);

    // msg("Module #%us:%n"
    //     "\tName: (%X2) %s%n"
    //     "\tCommand-line: (%X2) %s%n"
    //     "\tPhysical Address: %XP%n"
    //     "\tSize: %X4 (%Xs)%n"
    //     , index
    //     , module->name, JG_INFO_STRING_EX + module->name
    //     , module->cmdline, JG_INFO_STRING_EX + module->cmdline
    //     , module->address
    //     , module->length, size);

    ASSERTX(!PmmArc::MainAllocator->ContainsRange(paddr_t(module->address), psize_t(module->length)))
        (module->address)(module->length)XEND;

    vaddr_t vaddr = nullvaddr;

    res = Vmm::AllocatePages(&BootstrapProcess
        , size
        , MemoryAllocationOptions::Reserve | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global
        , MemoryContent::BootModule
        , vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to reserve %Xs bytes of kernel heap space for module #%us (%s): %H."
        , size
        , index, JG_INFO_STRING_EX + module->name
        , res);

    res = Vmm::MapRange(&BootstrapProcess
        , vaddr, paddr_t(module->address), size
        , MemoryFlags::Global
        , MemoryMapOptions::NoReferenceCounting);

    ASSERT(res.IsOkayResult()
        , "Failed to map range at %Xp to %XP (%Xs bytes) for module #%us (%s): %H."
        , vaddr, module->address, size
        , index, JG_INFO_STRING_EX + module->name
        , res);

    if (memeq("kernel64", JG_INFO_STRING_EX + module->name, 9))
        return KernelImage::Initialize(vaddr, size);
    else if (memeq("initrd", JG_INFO_STRING_EX + module->name, 7))
        return InitRd::Initialize(vaddr, size);

    return res;
}

/**
 *  <summary>
 *  Initializes the kernel modules.
 *  </summary>
 */
Handle Beelzebub::InitializeModules()
{
    Handle res;

    size_t const moduleCount = (size_t)JG_INFO_ROOT_EX->module_count;

    for (size_t i = 0; i < moduleCount; ++i)
    {
        res = HandleModule(i, JG_INFO_MODULE_EX + i);

        //  TODO: Perhaps handle some (non-)fatal errors here..?
    }

    return res;
}

/***************
    CPU DATA
***************/

#ifdef __BEELZEBUB__TEST_MT

// Thread tTa1;
// Thread tTa2;
// Thread tTa3;

// Thread tTb1;
// Thread tTb2;
// Thread tTb3;

// Process tPb;

// __hot void * TestThreadEntryPoint(void * const arg)
// {
//     (void)arg;

//     char * const myChars = (char *)(uintptr_t)0x321000ULL;

//     while (true)
//     {
//         Thread * activeThread = Cpu::GetThread();
//         char volatile specialChar = *myChars;

//         MSG_("Printing from thread %Xp! (%c)%n", activeThread, specialChar);

//         while (true) CpuInstructions::Halt();
//     }
// }

// __startup void InitializeTestThread(Thread * const t, Process * const p)
// {
//     new (t) Thread(p);

//     Handle res;

//     uintptr_t stackVaddr = nullvaddr;

//     res = Vmm::AllocatePages(CpuDataSetUp ? Cpu::GetProcess() : &BootstrapProcess
//         , 3 * PageSize
//         , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
//         , MemoryFlags::Global | MemoryFlags::Writable
//         , MemoryContent::ThreadStack
//         , stackVaddr);

//     ASSERT(res.IsOkayResult()
//         , "Failed to allocate stack for test thread: %H."
//         , res);

//     t->KernelStackTop = stackVaddr + 3 * PageSize;
//     t->KernelStackBottom = stackVaddr;

//     t->EntryPoint = &TestThreadEntryPoint;

//     InitializeThreadState(t);
//     //  This sets up the thread so it goes directly to the entry point when switched to.
// }

// __startup char * AllocateTestPage(Process * const p)
// {
//     Handle res;
//     //  Intermediate results.

//     vaddr_t const vaddr1 = 0x321000;
//     vaddr_t vaddr2 = nullvaddr;
//     paddr_t const paddr = Pmm::AllocateFrame();
//     //  Test page.

//     ASSERT(paddr != nullpaddr
//         , "Unable to allocate a physical page for test page of process %Xp!"
//         , p);

//     res = Vmm::AllocatePages(&BootstrapProcess
//         , 1 * PageSize
//         , MemoryAllocationOptions::Reserve | MemoryAllocationOptions::VirtualKernelHeap
//         , MemoryFlags::Global | MemoryFlags::Writable
//         , MemoryContent::Generic
//         , vaddr2);

//     ASSERT(res.IsOkayResult()
//         , "Failed to reserve test page: %H."
//         , res);

//     res = Vmm::MapPage(p, vaddr1, paddr, MemoryFlags::Writable);

//     ASSERT(res.IsOkayResult()
//         , "Failed to map page at %Xp (%XP) as test page in owning process: %H."
//         , vaddr1, paddr
//         , res);

//     res = Vmm::MapPage(&BootstrapProcess, vaddr2, paddr
//         , MemoryFlags::Global | MemoryFlags::Writable);

//     ASSERT(res.IsOkayResult()
//         , "Failed to map page at %Xp (%XP) as test page with boostrap memory manager: %H."
//         , vaddr2, paddr
//         , res);

//     return (char *)(uintptr_t)vaddr2;
// }

void Beelzebub::StartMultitaskingTest()
{
    // new (&tPb) Process();
    // //  Initialize a new process for thread series B.

    // Vmm::Initialize(&tPb);

    // char * tCa = AllocateTestPage(&BootstrapProcess);
    // //  Allocate and map a page for process B.

    // char * tCb = AllocateTestPage(&tPb);
    // //  Allocate and map a page for process B.

    // *tCa = 'A';
    // *tCb = 'B';
    // //  Assign different values.

    // InitializeTestThread(&tTa1, &BootstrapProcess);
    // InitializeTestThread(&tTa2, &BootstrapProcess);
    // InitializeTestThread(&tTa3, &BootstrapProcess);
    // //  Initialize thread series A under the bootstrap process.

    // InitializeTestThread(&tTb1, &tPb);
    // InitializeTestThread(&tTb2, &tPb);
    // InitializeTestThread(&tTb3, &tPb);
    // //  Initialize thread series B under their own process.

    // withInterrupts (false)
    // {
    //     // BootstrapThread.IntroduceNext(&tTb3);
    //     BootstrapThread.IntroduceNext(&tTb2);
    //     //  Threads B2 and B3 are at the end of the cycle.

    //     BootstrapThread.IntroduceNext(&tTa3);
    //     BootstrapThread.IntroduceNext(&tTb1);
    //     //  Threads B1 and A3 are intertwined.

    //     BootstrapThread.IntroduceNext(&tTa2);
    //     BootstrapThread.IntroduceNext(&tTa1);
    //     //  Threads A1 and A2 are at the start, right after the bootstrap thread.
    // }

    #ifdef __BEELZEBUB__TEST_APP
    if (CHECK_TEST(APP))
        TestApplication();
    #endif
}

#endif
