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

#include <cores.hpp>
#include <memory/vmm.hpp>
#include <system/cpu.hpp>
#include <synchronization/atomic.hpp>
#include <kernel.hpp>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

/****************
    Internals
****************/

static constexpr size_t const DataSize = RoundUp(sizeof(CpuData), __alignof(CpuData));
static uintptr_t DatasBase;

static Atomic<size_t> RegistrationCounter {0};

#if defined(__BEELZEBUB_SETTINGS_SMP)
    Atomic<uint16_t> TssSegmentCounter {(uint16_t)(8 * 8)};
#else
    CpuData BspCpuData;
#endif

static __startup void CreateStacks(CpuData * const data);

/******************
    Cores class
******************/

/*  Static  */

#if defined(__BEELZEBUB_SETTINGS_SMP)
size_t Cores::Count = 0;
#endif

bool Cores::Ready = false;

/*  Initialization  */

Handle Cores::Initialize(size_t const count)
{
    size_t const size = RoundUp(count * DataSize, PageSize);

    Handle res = Vmm::AllocatePages(nullptr
        , size / PageSize
        , MemoryAllocationOptions::Commit   | MemoryAllocationOptions::VirtualKernelHeap
        | MemoryAllocationOptions::GuardLow | MemoryAllocationOptions::GuardHigh
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::CpuDatas
        , DatasBase);

    assert_or(res.IsOkayResult()
        , "Failed to allocate pages to store CPU datas: %H."
        , res)
    {
        return res;
    }

    Count = count;

    return HandleResult::Okay;
}

void Cores::Register()
{
    size_t index = RegistrationCounter++;

    ASSERT(index < Count
        , "Too many CPU cores attempted to register with the cores manager!")
        (index)(Count);

    if (index == Count - 1)
        Ready = true;

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    CpuData * const data = reinterpret_cast<CpuData *>(DatasBase + index * DataSize);
    new (data) CpuData();

    data->Index = index;
    data->TssSegment = TssSegmentCounter.FetchAdd(sizeof(GdtTss64Entry));
#else
    CpuData * const data = &BspCpuData;
    new (data) CpuData();
    
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

    data->LastExtendedStateThread = nullptr;

    return CreateStacks(data);

    //msg("-- Core #%us @ %Xp. --%n", ind, data);
}

CpuData * Cores::Get(size_t const index)
{
    assert(index < Count)(index)(Count);

    return reinterpret_cast<CpuData *>(DatasBase + index * DataSize);
}

/****************
    Internals
****************/

void CreateStacks(CpuData * const data)
{
    //  NOTE:
    //  The first page is a guard page. Will triple-fault on overflow.

    Handle res;
    //  Intermediate results.

    //  First, the #DF stack.

    vaddr_t vaddr = nullvaddr;

    res = Vmm::AllocatePages(nullptr
        , DoubleFaultStackSize / PageSize
        , MemoryAllocationOptions::Commit   | MemoryAllocationOptions::VirtualKernelHeap
        | MemoryAllocationOptions::GuardLow | MemoryAllocationOptions::GuardHigh
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::ThreadStack
        , vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate #DF stack of CPU #%us: %H."
        , data->Index
        , res);

    data->EmbeddedTss.Ist[0] = vaddr + DoubleFaultStackSize;

    //  Then, the #PF stack.

    vaddr = nullvaddr;

    res = Vmm::AllocatePages(nullptr
        , PageFaultStackSize / PageSize
        , MemoryAllocationOptions::Commit   | MemoryAllocationOptions::VirtualKernelHeap
        | MemoryAllocationOptions::GuardLow | MemoryAllocationOptions::GuardHigh
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::ThreadStack
        , vaddr);

    ASSERT(res.IsOkayResult()
        , "Failed to allocate #DF stack of CPU #%us: %H."
        , data->Index
        , res);

    data->EmbeddedTss.Ist[1] = vaddr + PageFaultStackSize;
}
