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

#include <execution/process_image.hpp>
#include <kernel_data.hpp>
#include <execution/elf_default_mapper.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Terminals;

static Elf _ApplicationImage;

static bool HeaderValidator(ElfHeader1 const * header, void * data)
{
    return header->Identification.Class == ElfClass::Elf64;
}

/*************************
    ProcessImage class
*************************/

/*  Variables  */

Elf const * const ProcessImage::ApplicationImage = &_ApplicationImage;

/*  Initialization  */

Handle ProcessImage::Initialize()
{
    new (&_ApplicationImage) Elf(reinterpret_cast<void *>(STARTUP_DATA.MemoryImageStart), STARTUP_DATA.MemoryImageEnd - STARTUP_DATA.MemoryImageStart);

    ElfValidationResult evRes = _ApplicationImage.ValidateAndParse(&HeaderValidator, nullptr, nullptr);

    if (evRes != ElfValidationResult::Success)
    {
        DEBUG_TERM  << "Failed to validate and parse application image: "
                    << evRes;

        FAIL();
    }

    //  And map it.

    // DEBUG_TERM << _ApplicationImage << EndLine;

    evRes = _ApplicationImage.LoadAndValidate64(&MapSegment64, &UnmapSegment64, &ResolveSymbol, nullptr);

    if (evRes != ElfValidationResult::Success)
    {
        DEBUG_TERM  << "Failed to load application image: "
                    << evRes << Terminals::EndLine;

        FAIL();
    }

    //  TODO: Load dependencies, will need a filesystem and all those shenanigans. :(

    return HandleResult::Okay;
}

/*  Symbols  */

Elf::Symbol ProcessImage::ResolveSymbol(char const * name, void * lddata)
{
    Elf::Symbol res = ApplicationImage->GetSymbol(name);

    if (res.Defined)
        return res;

    //  So it wasn't defined in the app itself... It would be if the app were relocatable.
    //  Therefore, it must be in a dependency. With the current design, the runtime library takes highest priority amongst them.

    res = STARTUP_DATA.RuntimeImage.GetSymbol(name);

    if likely(res.Defined)
        return res;
    //  No? Ought to be in another dependency.

    //  TODO: Check other (loaded) dependencies.

    FAIL("Could not find symbol \"%s\" in the whole process.", name);
}
