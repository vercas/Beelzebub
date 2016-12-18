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

#include <lock_elision.hpp>
#include <system/code_patch.hpp>
#include <memory/vmm.hpp>
#include <system/cpu_instructions.hpp>
#include <kernel.hpp>
#include <entry.h>
#include <math.h>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;

__extern long const kernel_mapping_start;
__extern long const kernel_mapping_end;

struct LockAnnotation
{
    uintptr_t Start;
    uintptr_t End;
};

__extern LockAnnotation const locks_section_start;
__extern LockAnnotation const locks_section_end;

Handle Beelzebub::ElideLocks()
{
    if (&locks_section_start == &locks_section_end)
        return HandleResult::Okay;

    InterruptGuard<false> intGuard;

    Handle res;

    //  Step 1 is backing up the flags of all the pages, and making them
    //  writable, if they were not already.

    size_t const kernel_size = RoundUp(reinterpret_cast<uintptr_t>(&kernel_mapping_end) - reinterpret_cast<uintptr_t>(&kernel_mapping_start), PageSize);
    size_t const kernel_page_count = kernel_size / PageSize;

    msg("Kernel start @ %Xp, end @ %Xp, size %us, page count %us."
        , &kernel_mapping_start, &kernel_mapping_end
        , kernel_size, kernel_page_count);

    __extension__ MemoryFlags flags[kernel_page_count] {};

    for (size_t pageInd = 0; pageInd < kernel_page_count; ++pageInd)
    {
        vaddr_t const vaddr = reinterpret_cast<vaddr_t>(&kernel_mapping_start) + pageInd * PageSize;

        res = Vmm::GetPageFlags(&BootstrapProcess, vaddr, flags[pageInd]);

        assert_or(res.IsOkayResult()
            , "Failed to retrieve flags of page %Xp for lock elision: %H"
            , vaddr, res)
        {
            return res;
        }

        res = Vmm::SetPageFlags(&BootstrapProcess, vaddr, MemoryFlags::Writable | flags[pageInd]);

        assert_or(res.IsOkayResult()
            , "Failed to apply flags to page %Xp for lock elision: %H"
            , vaddr, res)
        {
            return res;
        }
    }

    //  Step 2 is performing the actual code patches.

    size_t const cacheLineSize = BootstrapCpuid.GetClflushLineSize();

    //  Finally, no-op the code.

    LockAnnotation const * cursor = &locks_section_start;

    for (/* nothing */; cursor < &locks_section_end; ++cursor)
    {
        bool okay = TurnIntoNoOp(reinterpret_cast<void *>(cursor->Start)
            , reinterpret_cast<void *>(cursor->End), true);

        assert_or(okay, "Failed to turn region %Xp-%Xp into a no-op."
            , cursor->Start, cursor->End)
        {
            return HandleResult::Failed;
        }

        for (uintptr_t i = RoundDown(cursor->Start, cacheLineSize); i < cursor->End; i += cacheLineSize)
            CpuInstructions::FlushCache((void *)i);
        //  The cache ought to be flushed, just to be on the safe side.
    }

    //  And step 3 is restoring the page flags.

    for (size_t pageInd = 0; pageInd < kernel_page_count; ++pageInd)
    {
        vaddr_t const vaddr = reinterpret_cast<vaddr_t>(&kernel_mapping_start) + pageInd * PageSize;

        res = Vmm::SetPageFlags(&BootstrapProcess, vaddr, flags[pageInd]);

        assert_or(res.IsOkayResult()
            , "Failed to restore flags to page %Xp for lock elision: %H"
            , vaddr, res)
        {
            return res;
        }
    }

    return HandleResult::Okay;
}
