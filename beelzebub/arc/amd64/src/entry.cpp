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

#include <system/cpuid.hpp>
#include <execution/thread_init.hpp>
#include <terminals/vbe.hpp>

#include <memory/vmm.hpp>
#include <memory/vmm.arc.hpp>

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    #include <memory/object_allocator_smp.hpp>
    #include <memory/object_allocator_pools_heap.hpp>
#endif

#include <keyboard.hpp>
#include <system/serial_ports.hpp>
#include <system/timers/pit.hpp>
#include <system/cpu.hpp>

#include <execution/images.hpp>

#include <kernel.hpp>
#include <entry.h>
#include <global_options.hpp>

#include <synchronization/atomic.hpp>
#include <math.h>
#include <string.h>

#ifdef __BEELZEBUB__TEST_APP
#include <tests/app.hpp>
#endif

#include <debug.hpp>
#include <_print/registers.hpp>
#include <_print/gdt.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;
using namespace Beelzebub::System::Timers;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Terminals;

CpuId Beelzebub::BootstrapCpuid;

/*********************
    CPU DATA STUBS
*********************/

//  No fancy allocation is needed on non-SMP systems.

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    ObjectAllocatorSmp CpuDataAllocator;
    Atomic<size_t> CpuIndexCounter {(size_t)0};
    Atomic<uint16_t> TssSegmentCounter {(uint16_t)(8 * 8)};
#else
    CpuData BspCpuData;
#endif

void InitializeCpuData(bool const bsp);
void InitializeCpuStacks(bool const bsp);

/*******************
    ENTRY POINTS
*******************/

void kmain_bsp()
{
#if   defined(__BEELZEBUB_SETTINGS_SMP)
    Cpu::Count = 1;
#endif

    Beelzebub::Main();
}

#if   defined(__BEELZEBUB_SETTINGS_SMP)
void kmain_ap()
{
    Interrupts::Register.Activate();

    ++Cpu::Count;

    Vmm::Switch(nullptr, &BootstrapProcess);
    //  Perfectly valid solution. Just to make sure.

    ++BootstrapProcess.ActiveCoreCount;

    InitializeCpuData(false);

    Beelzebub::Secondary();
}
#endif

/*****************************
    COMMAND-LINE ARGUMENTS
*****************************/

Handle ParseKernelArguments()
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

bool firstRegionCreated = false;

psize_t currentSpaceIndex = nullpaddr, currentSpaceLimit = nullpaddr;

PageAllocationSpace * currentAllocationSpacePtr = nullptr;
//  This one's used to instantiate allocation spaces.

PageAllocationSpace mainAllocationSpace;
PageAllocator mainAllocator;

//  NOTE: These variables are to be carefully reset for every domain.

/** 
 *  <summary>Creates an allocation space over the given physical range.</summary>
 */
__cold PageAllocationSpace * CreateAllocationSpace(paddr_t start, paddr_t end, Domain * domain)
{
    /*msg("ALLOC SPACE: %XP-%XP; ", start, end);//*/

    if (firstRegionCreated)
    {
        if (currentSpaceIndex == currentSpaceLimit)
        {
            //  The limit of the current allocation space location has
            //  been reached.

            currentSpaceIndex = 0;
            //  Reset the index.

            currentAllocationSpacePtr = nullptr;
            //  Nullify the location so it is recreated.

            /*msg("MAX; ");//*/
        }

        if (currentAllocationSpacePtr == nullptr)
        {
            PageDescriptor * desc = nullptr;
            paddr_t const paddr = domain->PhysicalAllocator->AllocatePage(PageAllocationOptions::GeneralPages, desc);

            ASSERT(paddr != nullpaddr && desc != nullptr
                , "Unable to allocate a special page for creating more allocation spaces!");

            Handle res = domain->PhysicalAllocator->ReserveByteRange(paddr, PageSize, PageReservationOptions::IncludeInUse);

            ASSERT(res.IsOkayResult()
                , "Failed to reserve special page for further allocation space creation: %H"
                , res);

            currentAllocationSpacePtr = reinterpret_cast<PageAllocationSpace *>((uintptr_t)paddr);

            /*msg("PAGE@%Xp; ", currentAllocationSpacePtr);//*/
        }

        /*msg("SPA@%Xp; I=%u2; "
            , currentAllocationSpacePtr + currentSpaceIndex
            , (uint16_t)currentSpaceIndex);//*/

        PageAllocationSpace * space = new (currentAllocationSpacePtr + currentSpaceIndex++) PageAllocationSpace(start, end, PageSize);
        //  One of the really neat things I like about C++.

        space->InitializeControlStructures();

        domain->PhysicalAllocator->PreppendAllocationSpace(space);

        /*msg("%XP-%XP;%n"
            , space->GetAllocationStart()
            , space->GetAllocationEnd());//*/

        return space;
    }
    else
    {
        new (&mainAllocationSpace) PageAllocationSpace(start, end, PageSize);

        mainAllocationSpace.InitializeControlStructures();

        new (domain->PhysicalAllocator = &mainAllocator) PageAllocator(&mainAllocationSpace);

        firstRegionCreated = true;

        currentSpaceIndex = 0;
        currentSpaceLimit = PageSize / sizeof(PageAllocationSpace);
        currentAllocationSpacePtr = nullptr;

        /*msg("FIRST; SPA@%Xp; ALC@%Xp; %XP-%XP; M=%u2,S=%u2%n"
            , &mainAllocationSpace, &mainAllocator
            , mainAllocationSpace.GetAllocationStart()
            , mainAllocationSpace.GetAllocationEnd()
            , (uint16_t)currentSpaceLimit
            , (uint16_t)sizeof(PageAllocationSpace));//*/

        return &mainAllocationSpace;
    }
}

/**
 *  <summary>
 *  Sanitizes the memory map and initializes the page allocator over the local
 *  region's available entries.
 *  </summary>
 */
Handle InitializePhysicalAllocator(jg_info_mmap_t * map // TODO: Don't depend on Jegudiel; let Jegudiel depend on Beelzebub!
                                 , size_t cnt
                                 , uintptr_t freeStart
                                 , Domain * domain)
{
    Handle res; //  Used for intermediary results.

    //  First step is aligning the memory map.
    //  Also, yes, I could've used bits 'n powers of two here.
    //  But I'm hoping to future-proof the code a bit, in case
    //  I would ever target a platform whose page size is not
    //  A power of two.
    //  Moreover, this code doesn't need to be lightning-fast. :L

    uint64_t start = RoundUp(freeStart, PageSize), end = 0;
    jg_info_mmap_t * firstMap = nullptr, * lastMap = nullptr;

    for (size_t i = 0; i < cnt; i++)
    {
        jg_info_mmap_t * m = map + i;
        //  Current map.

        if ((m->address + m->length) <= freeStart || !m->available)
            continue;

        if (firstMap == nullptr)
            firstMap = m;

        uint64_t mEnd = m->address + m->length;

        if (mEnd > end)
        {
            end = mEnd;
            lastMap = m;
        }
    }

    //  DUMPING MMAP ?

#ifdef __BEELZEBUB__DEBUG
    if (domain == &Domain0)
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
#endif

    //  SPACE CREATION

    for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
        if (m->available != 0 && m->length >= (2 * PageSize))
        {
            if (m->address < start && (m->address + m->length) > start)
                //  Means this entry crosses the start of free memory.
                CreateAllocationSpace(start, m->address + m->length, domain);
            else
                CreateAllocationSpace(m->address, m->address + m->length, domain);
        }

    //  PAGE RESERVATION

    for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
        if (m->available == 0)
        {
            res = domain->PhysicalAllocator->ReserveByteRange(m->address, m->length);

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
Handle InitializePhysicalMemory()
{
    Handle res;

    BootstrapCpuid = CpuId();
    BootstrapCpuid.Initialize();
    //  This is required to page all the available memory.

    BootstrapCpuid.PrintToTerminal(DebugTerminal);
    msg("%n");

    res = InitializePhysicalAllocator(JG_INFO_MMAP_EX, JG_INFO_ROOT_EX->mmap_count, JG_INFO_ROOT_EX->free_paddr, &Domain0);

    ASSERT(res.IsOkayResult()
        , "Failed to initialize the physical memory allocator for domain 0: %H.%n"
        , res);

    return HandleResult::Okay;
}

/*********************
    VIRTUAL MEMORY
*********************/

__cold __noinline void RemapTerminal(TerminalBase * const terminal);
//  Forward declaration.

/**
 *  <summary>
 *  Prepares the virtual address space to be used as intended by the kernel.
 *  </summary>
 */
Handle InitializeVirtualMemory()
{
    Handle res; //  Used for intermediary results.

    //  PAGING INITIALIZATION

    PageDescriptor * desc = nullptr;

    paddr_t const pml4_paddr = Domain0.PhysicalAllocator->AllocatePage(
        PageAllocationOptions::ThirtyTwoBit, desc);

    if (pml4_paddr == nullpaddr)
        return HandleResult::OutOfMemory;

    memset((void *)pml4_paddr, 0, PageSize);
    //  Clear it all out!
    desc->IncrementReferenceCount();
    //  Increment reference count...

    new (&BootstrapProcess) Process(pml4_paddr);

    VmmArc::Page1GB = BootstrapCpuid.CheckFeature(CpuFeature::Page1GB);
    VmmArc::NX      = BootstrapCpuid.CheckFeature(CpuFeature::NX     );

    Vmm::Bootstrap(&BootstrapProcess);
    ++BootstrapProcess.ActiveCoreCount;

    RemapTerminal(MainTerminal);

    if (MainTerminal != Debug::DebugTerminal)
        RemapTerminal(Debug::DebugTerminal);

    //  CPU DATA

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    new (&CpuDataAllocator) ObjectAllocatorSmp(sizeof(CpuData), __alignof(CpuData)
        , &AcquirePoolInKernelHeap, &EnlargePoolInKernelHeap, &ReleasePoolFromKernelHeap);
#endif

    InitializeCpuData(true);

    //  Now mapping the lower 16 MiB.

    for (size_t offset = 0; offset < VmmArc::IsaDmaLength; offset += PageSize)
    {
        vaddr_t const vaddr = VmmArc::IsaDmaStart + offset;
        paddr_t const paddr = (paddr_t)offset;

        res = Vmm::MapPage(&BootstrapProcess, vaddr, paddr
            , MemoryFlags::Global | MemoryFlags::Writable, PageDescriptor::Invalid);

        ASSERT(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP) for ISA DMA: %H."
            , vaddr, paddr
            , res);
    }

    //  TODO: Management for ISA DMA.

    return HandleResult::Okay;
}

void RemapTerminal(TerminalBase * const terminal)
{
    Handle res;

    if (terminal->Descriptor->Capabilities.Type == TerminalType::PixelMatrix)
    {
        VbeTerminal * const term = (VbeTerminal *)terminal;
        const size_t size = ((size_t)term->Pitch * (size_t)term->Height + PageSize - 1) & ~0xFFFULL;
        //  Yes, the size is aligned with page boundaries.

        if (term->VideoMemory >= VmmArc::KernelHeapStart
         && term->VideoMemory <  VmmArc::KernelHeapEnd)
            return;
        //  Nothing to do, folks.
        
        uintptr_t const loc = Vmm::KernelHeapCursor.FetchAdd(size);

        size_t off = 0;

        do
        {
            res = Vmm::MapPage(&BootstrapProcess, loc + off, term->VideoMemory + off
                , MemoryFlags::Global | MemoryFlags::Writable, PageDescriptor::Invalid);

            ASSERT(res.IsOkayResult()
                , "Failed to map page for VBE terminal (%Xp to %Xp): %H"
                , loc + off, term->VideoMemory + off
                , res);

            off += PageSize;
        } while (off < size);

        term->VideoMemory = loc;
    }

    //  TODO: Make a VGA text terminal and also handle it here.
}

/***********************
    PROCESSING UNITS
***********************/

/**
 *  <summary>
 *  Initializes the other processing units in the system.
 *  </summary>
 */
/*Handle InitializeAcpiTables()
{


    return HandleResult::Okay;
}//*/

/**
 *  <summary>
 *  Initializes the system memory for the boostrap processor.
 *  </summary>
 */
/*Handle InitializeMemory()
{
    BootstrapCpuid = CpuId();
    BootstrapCpuid.Initialize();
    //  This is required to page all the available memory.

    BootstrapCpuid.PrintToTerminal(DebugTerminal);
    msg("%n");

    InitializeVirtualMemory();

    msg("%n");

    //  DUMPING CONTROL REGISTERS

    PrintToDebugTerminal(Cpu::GetCr0());
    msg("%n");

    msg("CR4: %Xs%n", (uint64_t)Cpu::GetCr4());
    msg("%n");

    CpuInstructions::WriteBackAndInvalidateCache();

    auto GDTR = GdtRegister::Retrieve();

    PrintToDebugTerminal(GDTR);
    msg("%n%n");

    return HandleResult::Okay;
}//*/

/**************
    MODULES
**************/

/**
 *  <summary>
 *  Does something with the kernel's module...
 *  </summary>
 */
__cold Handle HandleKernelModule(size_t const index
                               , jg_info_module_t const * const module
                               , vaddr_t const vaddr
                               , size_t const size)
{
    msg("THIS IS THE KERNEL MODULE!%n");

    Image * kimg;

    Handle res = Images::Load("kernel", ImageRole::Kernel
        , reinterpret_cast<uint8_t *>(vaddr), size
        , kimg, nullptr);

    KernelImage = kimg;

    return res;
}

/**
 *  <summary>
 *  Processes a module.
 *  </summary>
 */
__cold Handle HandleModule(size_t const index, jg_info_module_t const * const module)
{
    Handle res = HandleResult::Okay;

    size_t const size = RoundUp(module->length, PageSize);

    msg("Module #%us:%n"
        "\tName: (%X2) %s%n"
        "\tCommand-line: (%X2) %s%n"
        "\tPhysical Address: %XP%n"
        "\tSize: %X4 (%Xs)%n"
        , index
        , module->name, JG_INFO_STRING_EX + module->name
        , module->cmdline, JG_INFO_STRING_EX + module->cmdline
        , module->address
        , module->length, size);//*/

    vaddr_t const vaddr = Vmm::KernelHeapCursor.FetchAdd(size);

    for (vaddr_t offset = 0; offset < size; offset += PageSize)
    {
        res = Vmm::MapPage(&BootstrapProcess, vaddr + offset, module->address + offset
            , MemoryFlags::Global | MemoryFlags::Writable, PageDescriptor::Invalid);

        ASSERT(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP) for module #%us (%s): %H."
            , vaddr + offset, module->address + offset
            , index, JG_INFO_STRING_EX + module->name
            , res);
    }

    if (memeq("kernel64", JG_INFO_STRING_EX + module->name, 9))
        return HandleKernelModule(index, module, vaddr, size);
#ifdef __BEELZEBUB__TEST_APP
    else if (memeq("loadtest", JG_INFO_STRING_EX + module->name, 9) && CHECK_TEST(APP))
        return HandleLoadtest(index, module, vaddr, size);
#endif

    return res;
}

/**
 *  <summary>
 *  Initializes the kernel modules.
 *  </summary>
 */
Handle InitializeModules()
{
    Handle res;

    Images::Initialize();

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

void InitializeCpuData(bool const bsp)
{
#if   defined(__BEELZEBUB_SETTINGS_SMP)
    CpuData * data = nullptr;

    Handle res = CpuDataAllocator.AllocateObject(data);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate CPU data structure! (%H)"
        , res);

    data->Index = CpuIndexCounter++;
    data->TssSegment = TssSegmentCounter.FetchAdd(sizeof(GdtTss64Entry));

    if (bsp)
        assert(data->Index == 0
            , "The BSP should have CPU index 0, not %us!"
            , data->Index);
#else
    CpuData * data = &BspCpuData;
    data->Index = 0;
    data->TssSegment = 8 * 8;
#endif

    data->SelfPointer = data;
    //  Hue.

    Msrs::Write(Msr::IA32_GS_BASE, (uint64_t)(uintptr_t)data);

    data->XContext = nullptr;
    //  TODO: Perhaps set up a default exception context, which would set fire
    //  to the whole system?

    data->DomainDescriptor = &Domain0;
    data->X2ApicMode = false;

    withLock (data->DomainDescriptor->GdtLock)
        data->DomainDescriptor->Gdt.Size = TssSegmentCounter.Load() - 1;
    //  This will eventually set the size to the highest value.

    data->DomainDescriptor->Gdt.Activate();
    //  Doesn't matter if a core lags behind here. It only needs its own TSS to
    //  be included.

    Gdt * gdt = data->DomainDescriptor->Gdt.Pointer;
    //  Pointer to the merry GDT.

    GdtTss64Entry & tssEntry = gdt->GetTss64(data->TssSegment);
    tssEntry = GdtTss64Entry()
    .SetSystemDescriptorType(GdtSystemEntryType::TssAvailable)
    .SetPresent(true)
    .SetBase(&(data->EmbeddedTss))
    .SetLimit((uint32_t)sizeof(struct Tss));

    CpuInstructions::Ltr(data->TssSegment);

    InitializeCpuStacks(bsp);

    //msg("-- Core #%us @ %Xp. --%n", ind, data);
}

void InitializeCpuStacks(bool const bsp)
{
    //  NOTE:
    //  The first page is a guard page. Will triple-fault on overflow.

    CpuData * cpuData = Cpu::GetData();

    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    //  First, the #DF stack.

    vaddr_t vaddr = Vmm::KernelHeapCursor.FetchAdd(DoubleFaultStackSize);

    for (size_t offset = PageSize; offset <= DoubleFaultStackSize; offset += PageSize)
    {
        paddr_t const paddr = mainAllocator.AllocatePage(desc);
        //  Stack page.

        ASSERT(paddr != nullpaddr && desc != nullptr
            , "Unable to allocate a physical page #%us for DF stack of CPU #%us!"
            , offset / PageSize - 1, cpuData->Index);

        res = Vmm::MapPage(&BootstrapProcess, vaddr + offset, paddr
            , MemoryFlags::Global | MemoryFlags::Writable, desc);

        ASSERT(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP) for DF stack of CPU #%us: %H."
            , vaddr + offset, paddr, cpuData->Index
            , res);
    }

    cpuData->EmbeddedTss.Ist[0] = vaddr + PageSize + DoubleFaultStackSize;

    //  Then, the #PF stack.

    vaddr = Vmm::KernelHeapCursor.FetchAdd(PageFaultStackSize + PageSize);

    for (size_t offset = PageSize; offset <= PageFaultStackSize; offset += PageSize)
    {
        paddr_t const paddr = mainAllocator.AllocatePage(desc);
        //  Stack page.

        ASSERT(paddr != nullpaddr && desc != nullptr
            , "Unable to allocate a physical page #%us for DF stack of CPU #%us!"
            , offset / PageSize - 1, cpuData->Index);

        res = Vmm::MapPage(&BootstrapProcess, vaddr + offset, paddr
            , MemoryFlags::Global | MemoryFlags::Writable, desc);

        ASSERT(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP) for DF stack of CPU #%us: %H."
            , vaddr + offset, paddr, cpuData->Index
            , res);
    }

    cpuData->EmbeddedTss.Ist[1] = vaddr + PageSize + PageFaultStackSize;
}

#ifdef __BEELZEBUB__TEST_MT

Thread tTa1;
Thread tTa2;
Thread tTa3;

Thread tTb1;
Thread tTb2;
Thread tTb3;

Process tPb;

__hot void * TestThreadEntryPoint(void * const arg)
{
    char * const myChars = (char *)(uintptr_t)0x321000ULL;

    while (true)
    {
        Thread * activeThread = Cpu::GetData()->ActiveThread;
        char volatile specialChar = *myChars;

        MSG_("Printing from thread %Xp! (%c)%n", activeThread, specialChar);

        while (true) CpuInstructions::Halt();
    }
}

__cold void InitializeTestThread(Thread * const t, Process * const p)
{
    new (t) Thread(p);

    Handle res;

    uintptr_t stackVaddr = nullvaddr;

    res = Vmm::AllocatePages(CpuDataSetUp ? Cpu::GetData()->ActiveThread->Owner : &BootstrapProcess
        , 3, MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global | MemoryFlags::Writable, stackVaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate stack for test thread: %H."
        , res);

    t->KernelStackTop = stackVaddr + 3 * PageSize;
    t->KernelStackBottom = stackVaddr;

    t->EntryPoint = &TestThreadEntryPoint;

    InitializeThreadState(t);
    //  This sets up the thread so it goes directly to the entry point when switched to.
}

__cold char * AllocateTestPage(Process * const p)
{
    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    vaddr_t const vaddr1 = 0x321000;
    vaddr_t const vaddr2 = Vmm::KernelHeapCursor.FetchAdd(PageSize);
    paddr_t const paddr = mainAllocator.AllocatePage(desc);
    //  Test page.

    ASSERT(paddr != nullpaddr && desc != nullptr
        , "Unable to allocate a physical page for test page of process %Xp!"
        , p);

    res = Vmm::MapPage(p, vaddr1, paddr, MemoryFlags::Writable, desc);

    ASSERT(res.IsOkayResult()
        , "Failed to map page at %Xp (%XP) as test page in owning process: %H."
        , vaddr1, paddr
        , res);

    res = Vmm::MapPage(&BootstrapProcess, vaddr2, paddr
        , MemoryFlags::Global | MemoryFlags::Writable, desc);

    ASSERT(res.IsOkayResult()
        , "Failed to map page at %Xp (%XP) as test page with boostrap memory manager: %H."
        , vaddr2, paddr
        , res);

    return (char *)(uintptr_t)vaddr2;
}

void StartMultitaskingTest()
{
    new (&tPb) Process();
    //  Initialize a new process for thread series B.

    Vmm::Initialize(&tPb);

    char * tCa = AllocateTestPage(&BootstrapProcess);
    //  Allocate and map a page for process B.

    char * tCb = AllocateTestPage(&tPb);
    //  Allocate and map a page for process B.

    *tCa = 'A';
    *tCb = 'B';
    //  Assign different values.

    InitializeTestThread(&tTa1, &BootstrapProcess);
    InitializeTestThread(&tTa2, &BootstrapProcess);
    InitializeTestThread(&tTa3, &BootstrapProcess);
    //  Initialize thread series A under the bootstrap process.

    InitializeTestThread(&tTb1, &tPb);
    InitializeTestThread(&tTb2, &tPb);
    InitializeTestThread(&tTb3, &tPb);
    //  Initialize thread series B under their own process.

    withInterrupts (false)
    {
        // BootstrapThread.IntroduceNext(&tTb3);
        BootstrapThread.IntroduceNext(&tTb2);
        //  Threads B2 and B3 are at the end of the cycle.

        BootstrapThread.IntroduceNext(&tTa3);
        BootstrapThread.IntroduceNext(&tTb1);
        //  Threads B1 and A3 are intertwined.

        BootstrapThread.IntroduceNext(&tTa2);
        BootstrapThread.IntroduceNext(&tTa1);
        //  Threads A1 and A2 are at the start, right after the bootstrap thread.
    }

    #ifdef __BEELZEBUB__TEST_APP
    if (CHECK_TEST(APP))
        TestApplication();
    #endif
}

#endif
