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

#include <execution/elf_default_mapper.hpp>
#include <execution/elf.hpp>
#include <syscalls/memory.h>

#include <string.h>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

bool Execution::MapSegment64(uintptr_t loc, uintptr_t img, ElfProgramHeader_64 const & phdr, void * data)
{
    vaddr_t const segVaddr    = loc + RoundDown(phdr.VAddr, PageSize);
    vaddr_t const segVaddrEnd = loc + RoundUp  (phdr.VAddr + phdr.VSize, PageSize);

    // DEBUG_TERM  << "Requesting memory for segment: " << (void *)segVaddr << " - " << (void *)segVaddrEnd << Terminals::EndLine
    //             << "loc = " << (void *)loc << Terminals::EndLine
    //             << "img = " << (void *)img << Terminals::EndLine
    //             << "phdr.VAddr = " << (void *)phdr.VAddr << "; RD " << (void *)RoundDown(phdr.VAddr, PageSize) << Terminals::EndLine
    //             << "Segment start = " << (void *)(loc + RoundDown(phdr.VAddr, PageSize)) << Terminals::EndLine
    //             << "Segment end   = " << (void *)(loc + RoundUp(phdr.VAddr + phdr.VSize, PageSize)) << Terminals::EndLine;

    ASSERT(segVaddrEnd > segVaddr);

    MemoryRequestOptions mreqOpts = MemoryRequestOptions::Commit;

    if (0 != (phdr.Flags & ElfProgramHeaderFlags::Executable))
        mreqOpts |= MemoryRequestOptions::Executable;

    //if (0 != (phdr.Flags & ElfProgramHeaderFlags::Writable))
        mreqOpts |= MemoryRequestOptions::Writable;
    //  TODO: Syscall to change page flags.

    Handle res = MemoryRequest(segVaddr, segVaddrEnd - segVaddr, mreqOpts);
    auto resPtr = res.GetPage();

    if unlikely(resPtr == nullptr)
    {
        DEBUG_TERM  << "Failed to allocate memory for " << phdr << ": " << res
                    << Terminals::EndLine;

        return false;
    }
    else if unlikely((vaddr_t)resPtr != segVaddr)
    {
        DEBUG_TERM  << "Failed to allocate memory for " << phdr
                    << " at the requested location; was given " << resPtr
                    << " instead." << Terminals::EndLine;

        return false;
    }

    memcpy(reinterpret_cast<void *>(loc + phdr.VAddr )
        ,  reinterpret_cast<void *>(img + phdr.Offset), phdr.PSize);

    if (phdr.VSize > phdr.PSize)
        memset(reinterpret_cast<void *>(loc + phdr.VAddr + phdr.PSize)
            , 0, phdr.VSize - phdr.PSize);

    return true;
}

bool Execution::UnmapSegment64(uintptr_t loc, ElfProgramHeader_64 const & phdr, void * data)
{
    vaddr_t const segVaddr    = loc + RoundDown(phdr.VAddr, PageSize);
    vaddr_t const segVaddrEnd = loc + RoundUp  (phdr.VAddr + phdr.VSize, PageSize);

    if (segVaddrEnd <= segVaddr)
        return false;
    //  So it starts before the userland or ends after the kernel... Not good.

    Handle res = MemoryRelease(segVaddr, segVaddrEnd - segVaddr, MemoryReleaseOptions::None);

    return res.IsOkayResult();
}
