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

#include "cores.hpp"
#include "memory/vmm.hpp"
#include "system/cpu.hpp"
#include "kernel.image.hpp"
#include "kernel.hpp"
#include <beel/sync/atomic.hpp>
#include <math.h>
#include <string.h>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

/****************
    Internals
****************/

static size_t DataSize, TlsSize;
static uintptr_t DatasBase;

static Atomic<size_t> RegistrationCounter {0};

Atomic<uint16_t> TssSegmentCounter {(uint16_t)(8 * 8)};

static __startup void CreateStacks(CpuData * const data);

#ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_STREAMFLOW
    //  TODO: Unhax this thing.
    extern "C" __thread unsigned int thread_id;
#endif

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
    if (KernelImage::Elf.TLS_64 != nullptr)
    {
        auto & phdrTls = KernelImage::Elf.TLS_64;

        TlsSize = RoundUp(phdrTls->VSize, phdrTls->Alignment);
        DataSize = RoundUp(TlsSize + sizeof(CpuData), Maximum(__alignof(CpuData), phdrTls->Alignment));
    }
    else
    {
        TlsSize = 0;
        DataSize = RoundUp(sizeof(CpuData), __alignof(CpuData));
    }

    size_t const size = RoundUp(count * DataSize, PageSize);

    Handle res = Vmm::AllocatePages(nullptr
        , size
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

    ASSERTX(index < Count
        , "Too many CPU cores attempted to register with the cores manager!")
        (index)(Count)XEND;

    if (index == Count - 1)
        Ready = true;

    uint8_t * const loc = reinterpret_cast<uint8_t *>(DatasBase + index * DataSize);

    //  First, initialize the kernel's CPU data structure.

    CpuData * const data = reinterpret_cast<CpuData *>(loc + TlsSize);
    new (data) CpuData();

    data->Index = index;
    data->TssSegment = TssSegmentCounter.FetchAdd(sizeof(GdtTss64Entry));

    data->SelfPointer = data;
    //  Hue.

    //  Then, activate it.

    Msrs::Write(Msr::IA32_GS_BASE, (uint64_t)(uintptr_t)data);

    //  Now, the rest of the structure...

    data->DomainDescriptor = &Domain0;

    withLock (data->DomainDescriptor->GdtLock)
    {
        //  This will eventually set the size to the highest value.

        uint16_t cnt = TssSegmentCounter.Load();

        if (data->DomainDescriptor->Gdt.Size < cnt - 1)
            data->DomainDescriptor->Gdt.Size = cnt - 1;
    }

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

    //  And the stack.

    CreateStacks(data);

    //  And finally, prepare the TLS area!

    if (KernelImage::Elf.TLS_64 != nullptr)
    {
        auto & phdrTls = *(KernelImage::Elf.TLS_64);

        void * const loc2 = mempcpy(loc, reinterpret_cast<void *>(KernelImage::Elf.Start + phdrTls.Offset), phdrTls.PSize);

        memset(loc2, 0, phdrTls.VSize - phdrTls.PSize);

    #ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_STREAMFLOW
        //  TODO: Unhax this thing.
        thread_id = (unsigned int)index;
    #endif
    }
}

#ifdef __BEELZEBUB__CONF_DEBUG
void Cores::AssertCoreRegistration()
{
    uint64_t const addr = Msrs::Read(Msr::IA32_GS_BASE).Qword;

    ASSERT((addr - DatasBase) % DataSize == TlsSize, "Misaligned core data!")
        ("address", Terminals::Hexadecimal, addr)
        (DatasBase)(DataSize)(TlsSize)
        ("actual", (addr - DatasBase) % DataSize);

    ASSERT(addr >= DatasBase, "Core data before base address!")
        ("address", Terminals::Hexadecimal, addr)
        (DatasBase);

    ASSERT(addr < (DatasBase + (GetCount() * DataSize)), "Core data beyond end!")
        ("address", Terminals::Hexadecimal, addr)
        ("end", DatasBase + (GetCount() * DataSize));
}
#endif

CpuData * Cores::Get(size_t const index)
{
    assert(index < Count)(index)(Count);

    return reinterpret_cast<CpuData *>(DatasBase + index * DataSize + TlsSize);
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
        , DoubleFaultStackSize
        , MemoryAllocationOptions::Commit   | MemoryAllocationOptions::VirtualKernelHeap
        | MemoryAllocationOptions::GuardLow | MemoryAllocationOptions::GuardHigh
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::ThreadStack
        , vaddr);

    ASSERTX(res.IsOkayResult()
        , "Failed to allocate #DF stack of CPU #%us: %H."
        , data->Index
        , res)XEND;

    data->EmbeddedTss.Ist[0] = vaddr + DoubleFaultStackSize;

    //  Then, the #PF stack.

    vaddr = nullvaddr;

    res = Vmm::AllocatePages(nullptr
        , PageFaultStackSize
        , MemoryAllocationOptions::Commit   | MemoryAllocationOptions::VirtualKernelHeap
        | MemoryAllocationOptions::GuardLow | MemoryAllocationOptions::GuardHigh
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::ThreadStack
        , vaddr);

    ASSERTX(res.IsOkayResult()
        , "Failed to allocate #DF stack of CPU #%us: %H."
        , data->Index
        , res)XEND;

    data->EmbeddedTss.Ist[1] = vaddr + PageFaultStackSize;
}
