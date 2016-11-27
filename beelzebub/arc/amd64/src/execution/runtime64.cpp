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

#include <execution/runtime64.hpp>
#include <initrd.hpp>
#include <execution/elf_default_mapper.hpp>
#include <memory/vmm.hpp>
#include <system/cpu.hpp>
#include <kernel.hpp>

#include <string.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;

static bool HeaderValidator(ElfHeader1 const * header, void * data)
{
    return header->Identification.Class == ElfClass::Elf64;
}

/**********************
    Runtime64 class
**********************/

/*  Statics  */

Elf Runtime64::Template;

/*  Methods  */

Handle Runtime64::Initialize()
{
    if (!InitRd::Loaded)
        return HandleResult::UnsupportedOperation;

    Handle file = InitRd::FindItem("/usr/lib/libbeelzebub.amd64.so");

    if unlikely(!file.IsType(HandleType::InitRdFile))
        return file;

    FileBoundaries bnd = InitRd::GetFileBoundaries(file);

    if (bnd.Start == 0 || bnd.Size == 0)
        return HandleResult::IntegrityFailure;

    //  Now to finally do the mappin'.
    
    if (bnd.Start % PageSize == 0 && bnd.AlignedSize % PageSize == 0)
    {
        for (size_t offset = 0; offset < bnd.Size; offset += PageSize)
        {
            Handle res = Vmm::SetPageFlags(&BootstrapProcess, bnd.Start + offset
                , MemoryFlags::Global | MemoryFlags::Userland);
            //  Modules are normally global-supervisor-writable. This one needs to
            //  be global-userland-readable.

            ASSERT(res.IsOkayResult()
                , "Failed to change flags of page at %Xp for 64-bit runtime module: %H."
                , bnd.Start + offset, res);
        }

        new (&Template) Elf(reinterpret_cast<void *>(bnd.Start), bnd.Size);
    }
    else
    {
        vaddr_t vaddr = nullvaddr;
        size_t const size = bnd.Size;

        Handle res = Vmm::AllocatePages(&BootstrapProcess
            , RoundUp(size, PageSize)
            , MemoryAllocationOptions::Commit | MemoryAllocationOptions::VirtualKernelHeap
            , MemoryFlags::Global | MemoryFlags::Userland
            , MemoryContent::Generic
            , vaddr);

        ASSERT(res.IsOkayResult()
            , "Failed to allocate space for 64-bit runtime image: %H."
            , res);

        withWriteProtect (false)
            memcpy(reinterpret_cast<void *>(vaddr), reinterpret_cast<void *>(bnd.Start), size);

        new (&Template) Elf(reinterpret_cast<void *>(vaddr), size);
    }

    ElfValidationResult evRes = Template.ValidateAndParse(&HeaderValidator, nullptr, nullptr);

    if (evRes != ElfValidationResult::Success)
    {
        DEBUG_TERM_ << "Failed to validate and parse 64-bit runtime library: "
                    << evRes << Terminals::EndLine;

        FAIL();
    }

    return HandleResult::Okay;
}

Handle Runtime64::Deploy(uintptr_t base, StartupData * & data)
{
    Elf copy = Runtime64::Template;
    //  A copy is needed; the original mustn't be modified.

    //  Then, relocate the ELF.

    ElfValidationResult evRes = copy.Relocate(base);

    if (evRes != ElfValidationResult::Success)
    {
        DEBUG_TERM_ << "Failed to relocate 64-bit runtime library: " << evRes
                    << Terminals::EndLine;

        assert_or(false, "Failed to relocate 64-bit runtime.")
            return HandleResult::ImageRelocationFailure;
    }

    //  Then map the segments...

    evRes = copy.LoadAndValidate64(&MapSegment64, &UnmapSegment64, nullptr, nullptr);

    if (evRes != ElfValidationResult::Success)
    {
        DEBUG_TERM_ << "Failed to load 64-bit runtime library: " << evRes
                    << Terminals::EndLine;

        assert_or(false, "Failed to load 64-bit runtime.")
            return HandleResult::ImageLoadingFailure;
    }

    //  Then find a "Self" symbol.

    Elf::Symbol stdat_s = copy.GetSymbol(STARTUP_DATA_SYMBOL);

    assert_or(stdat_s.Exists, "'" STARTUP_DATA_SYMBOL "' symbol doesn't exist in the runtime.");

    assert_or(stdat_s.Size == sizeof(StartupData)
        , "'" STARTUP_DATA_SYMBOL "' symbol size doesn't match ELF class size!")
        return HandleResult::RuntimeMismatch;

    assert_or(copy.CheckRangeLoaded64(stdat_s.Value - base, stdat_s.Size, RangeLoadOptions::Writable) == RangeLoadStatus::FullyLoaded
        , "'" STARTUP_DATA_SYMBOL "' symbol is not within a writable loaded section!")
        return HandleResult::RuntimeMismatch;

    //  Finally, fill in the blanks.

    StartupData * stdat = reinterpret_cast<StartupData *>(stdat_s.Value);

    stdat->RuntimeImage = copy;
    //  Aye, copy the ELF class into the userland.

    data = stdat;

    return HandleResult::Okay;
}
