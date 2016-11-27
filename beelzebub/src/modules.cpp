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

#include <modules.hpp>
#include <execution/elf.kmod.mapper.hpp>
#include <memory/object_allocator_smp.hpp>
#include <memory/object_allocator_pools_heap.hpp>
#include <memory/vmm.hpp>

#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;

struct KernelModule
{
    Elf Image;
};

typedef HandlePointer<KernelModule, HandleType::KernelModule, 0> KernelModuleHandle;

ObjectAllocatorSmp ModulesAllocator;

static bool HeaderValidator(ElfHeader1 const * header, void * data)
{
    return header->Identification.Class == ElfClass::Elf64;
}

/********************
    Modules class
********************/

/*  Statics  */

bool Modules::Initialized = false;

/*  Initialization  */

Handle Modules::Initialize()
{
    new (&ModulesAllocator) ObjectAllocatorSmp(sizeof(KernelModule), __alignof(KernelModule)
        , &AcquirePoolInKernelHeap, &EnlargePoolInKernelHeap, &ReleasePoolFromKernelHeap);

    Modules::Initialized = true;

    return HandleResult::Okay;
}

/*  (Un)loading  */

Handle Modules::Load(uintptr_t start, size_t len)
{
    KernelModule * kmod;
    Handle res = ModulesAllocator.AllocateObject(kmod);

    if (!res.IsOkayResult())
        return res;

    kmod->Image = Elf(start, len);

    ElfValidationResult evRes = kmod->Image.ValidateAndParse(&HeaderValidator, nullptr, nullptr);

    if (evRes != ElfValidationResult::Success)
        return HandleResult::Failed;

    //  So, the ELF file is parsed.

    size_t const size = RoundUp(kmod->Image.GetSizeInMemory(), PageSize);
    vaddr_t base = nullvaddr;

    res = Vmm::AllocatePages(nullptr
        , size
        , MemoryAllocationOptions::Commit   | MemoryAllocationOptions::VirtualKernelHeap
        | MemoryAllocationOptions::GuardLow | MemoryAllocationOptions::GuardHigh
        , MemoryFlags::Writable | MemoryFlags::Executable | MemoryFlags::Global
        , MemoryContent::KernelModule
        , base);

    if (!res.IsOkayResult())
        return res;

    //  Space is reserved for it.

    evRes = kmod->Image.Relocate(base);

    if (evRes != ElfValidationResult::Success)
        return HandleResult::ImageRelocationFailure;

    //  It's relocated...

    evRes = kmod->Image.LoadAndValidate64(&MapKmodSegment64, &UnmapKmodSegment64, nullptr, nullptr);

    if (evRes != ElfValidationResult::Success)
        return HandleResult::ImageLoadingFailure;

    //  And properly loaded!

    return KernelModuleHandle(kmod).ToHandle(true);
}
