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
#include <execution/elf_default_mapper.hpp>
#include <memory/vmm.hpp>
#include <kernel.hpp>
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

Handle Runtime64::HandleTemplate(size_t index, jg_info_module_t const * module
                               , vaddr_t vaddr, size_t size)
{
    for (size_t offset = 0; offset < size; offset += PageSize)
    {
        Handle res = Vmm::SetPageFlags(&BootstrapProcess, vaddr + offset
            , MemoryFlags::Global | MemoryFlags::Userland);
        //  Modules are normally global-supervisor-writable. This one needs to
        //  be global-userland-readable.

        ASSERT(res.IsOkayResult()
            , "Failed to change flags of page at %Xp for 64-bit runtime module: %H."
            , vaddr + offset, res);
    }

    new (&Template) Elf(reinterpret_cast<void *>(vaddr), size);

    ElfValidationResult evRes = Template.ValidateAndParse(&HeaderValidator, nullptr, nullptr);

    if (evRes != ElfValidationResult::Success)
    {
        DEBUG_TERM_ << "Failed to validate and parse 64-bit runtime library: "
                    << evRes << Terminals::EndLine;

        ASSERT(false);
    }

    return HandleResult::Okay;
}

Handle Runtime64::Deploy(uintptr_t base)
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

    evRes = copy.LoadAndValidate64(&MapSegment64, &UnmapSegment64, nullptr);

    if (evRes != ElfValidationResult::Success)
    {
        DEBUG_TERM_ << "Failed to load 64-bit runtime library: " << evRes
                    << Terminals::EndLine;

        assert_or(false, "Failed to load 64-bit runtime.")
            return HandleResult::ImageLoadingFailure;
    }

    //  Then find a "Self" symbol.

    Elf::Symbol self = copy.GetSymbol("Self");

    assert_or(self.Exists, "'Self' symbol doesn't exist in the runtime.");

    assert_or(self.Size == sizeof(Elf)
        , "'Self' symbol size doesn't match ELF class size!")
        return HandleResult::RuntimeMismatch;

    assert_or(copy.CheckRangeLoaded64(self.Value - base, self.Size, RangeLoadOptions::Writable) == RangeLoadStatus::FullyLoaded
        , "'Self' symbol is not within a writable loaded section!")
        return HandleResult::RuntimeMismatch;

    Elf * copyDestination = reinterpret_cast<Elf *>(self.Value);

    *copyDestination = copy;
    //  Aye, copy the ELF class into the userland.

    return HandleResult::Okay;
}
