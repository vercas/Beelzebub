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
#include <math.h>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;

struct LockAnnotation
{
    uintptr_t Start;
    uintptr_t End;
};

__extern LockAnnotation const locks_section_start;
__extern LockAnnotation const locks_section_end;

Handle Beelzebub::ElideLocks()
{
    Handle res;

    msg("LOCKS start @ %Xp, end @ %Xp%n"
        , &locks_section_start, &locks_section_end);

    LockAnnotation const * cursor = &locks_section_start;

    for (/* nothing */; cursor < &locks_section_end; ++cursor)
        msg("Lock: %Xp -> %Xp%n", cursor->Start, cursor->End);

    cursor = &locks_section_start;

    for (/* nothing */; cursor < &locks_section_end; ++cursor)
    {
        Debug::DebugTerminal->WriteHexDump(cursor->Start, cursor->End - cursor->Start, 16);
        msg("Lock: %Xp -> %Xp ", cursor->Start, cursor->End);

        vaddr_t const vaddr1 = RoundDown(cursor->Start, PageSize);
        vaddr_t const vaddr2 = RoundDown(cursor->End  , PageSize);

        //  First, get the page flags.

        MemoryFlags mf1 = MemoryFlags::None, mf2 = MemoryFlags::None;

        res = Vmm::GetPageFlags(&BootstrapProcess, vaddr1, mf1);

        assert_or(res.IsOkayResult()
            , "Failed to retrieve flags of page %Xp for lock elision: %H"
            , vaddr1, res)
        {
            return res;
        }

        if (vaddr2 != vaddr1)
        {
            res = Vmm::GetPageFlags(&BootstrapProcess, vaddr2, mf2);

            assert_or(res.IsOkayResult()
                , "Failed to retrieve flags of page %Xp for lock elision: %H"
                , vaddr2, res)
            {
                return res;
            }
        }

        msg("A ");

        //  Then make the pages writable and executable (because they may overlap
        //  with this code and all the functions used by it).

        res = Vmm::SetPageFlags(&BootstrapProcess, vaddr1, MemoryFlags::Writable | MemoryFlags::Executable | MemoryFlags::Global);

        assert_or(res.IsOkayResult()
            , "Failed to apply flags to page %Xp for lock elision: %H"
            , vaddr1, res)
        {
            return res;
        }

        if (vaddr2 != vaddr1)
        {
            res = Vmm::SetPageFlags(&BootstrapProcess, vaddr2, MemoryFlags::Writable | MemoryFlags::Executable | MemoryFlags::Global);

            assert_or(res.IsOkayResult()
                , "Failed to apply flags to page %Xp for lock elision: %H"
                , vaddr2, res)
            {
                return res;
            }
        }

        msg("B%n");

        //  Finally, no-op the code.

        bool okay = TurnIntoNoOp(reinterpret_cast<void *>(cursor->Start)
            , reinterpret_cast<void *>(cursor->End), true);

        assert_or(okay, "Failed to turn region %Xp-%Xp into a no-op."
            , cursor->Start, cursor->End)
        {
            return HandleResult::Failed;
        }

        Debug::DebugTerminal->WriteLine();
        Debug::DebugTerminal->WriteHexDump(cursor->Start, cursor->End - cursor->Start, 16);

        //  And close by restoring the page flags.

        res = Vmm::SetPageFlags(&BootstrapProcess, vaddr1, mf1);

        assert_or(res.IsOkayResult()
            , "Failed to apply flags to page %Xp for lock elision: %H"
            , vaddr1, res)
        {
            return res;
        }

        if (vaddr2 != vaddr1)
        {
            res = Vmm::SetPageFlags(&BootstrapProcess, vaddr2, mf2);

            assert_or(res.IsOkayResult()
                , "Failed to apply flags to page %Xp for lock elision: %H"
                , vaddr2, res)
            {
                return res;
            }
        }

        msg("D ");

        for (uintptr_t i = cursor->Start; i < cursor->End; i += 32)
            CpuInstructions::FlushCache((void *)i);

        msg("E%n");
    }

    return HandleResult::Okay;
}
