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

#ifdef __BEELZEBUB__TEST_LOCK_ELISION

#include <tests/lock_elision.hpp>
#include <system/code_patch.hpp>
#include <memory/vmm.hpp>
#include <system/cpu_instructions.hpp>
#include <kernel.hpp>
#include <math.h>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;

__extern __section(data) uintptr_t testStart1;
__extern __section(data) uintptr_t testEnd1;

__startup void TestFunction1()
{
    uint8_t volatile tVar = 0;
    msg("pre ");

op_start:
    msg("mid ");
op_end:

    msg("post %u1%n", tVar);

    asm volatile goto ( ".pushsection .data \n\t"
                        ".global testStart1 \n\t"
                        "testStart1: " _GAS_DATA_POINTER " %l0 \n\t"
                        ".global testEnd1 \n\t"
                        "testEnd1: " _GAS_DATA_POINTER " %l1 \n\t"
                        ".popsection \n\t"
                        : : : : op_start, op_end );
}

__extern __section(data) uintptr_t testStart2;
__extern __section(data) uintptr_t testEnd2;

__startup void TestFunction2()
{
    uint8_t volatile tVar = 0;
    msg("PRE %u1 ", tVar);

op_start:
    tVar = 1;
op_end:

    msg("POST %u1%n", tVar);

    asm volatile goto ( ".pushsection .data \n\t"
                        ".global testStart2 \n\t"
                        "testStart2: " _GAS_DATA_POINTER " %l0 \n\t"
                        ".global testEnd2 \n\t"
                        "testEnd2: " _GAS_DATA_POINTER " %l1 \n\t"
                        ".popsection \n\t"
                        : : : : op_start, op_end );
}

__startup void PatchRange(uintptr_t tStart, uintptr_t tEnd)
{
    // Debug::DebugTerminal->WriteHexDump(tStart, tEnd - tStart, 16);
    // msg("Lock: %Xp -> %Xp ", tStart, tEnd);

    vaddr_t const vaddr1 = RoundDown(tStart, PageSize);
    vaddr_t const vaddr2 = RoundDown(tEnd  , PageSize);

    //  First, get the page flags.

    MemoryFlags mf1 = MemoryFlags::None, mf2 = MemoryFlags::None;

    Handle res = Vmm::GetPageFlags(nullptr, vaddr1, mf1);

    ASSERT(res.IsOkayResult()
        , "Failed to retrieve flags of page %Xp for lock elision: %H"
        , vaddr1, res);

    if (vaddr2 != vaddr1)
    {
        res = Vmm::GetPageFlags(nullptr, vaddr2, mf2);

        ASSERT(res.IsOkayResult()
            , "Failed to retrieve flags of page %Xp for lock elision: %H"
            , vaddr2, res);
    }

    // msg("A ");

    //  Then make the pages writable and executable (because they may overlap
    //  with this code and all the functions used by it).

    res = Vmm::SetPageFlags(nullptr, vaddr1, MemoryFlags::Writable | MemoryFlags::Executable | MemoryFlags::Global);

    ASSERT(res.IsOkayResult()
        , "Failed to apply flags to page %Xp for lock elision: %H"
        , vaddr1, res);

    if (vaddr2 != vaddr1)
    {
        res = Vmm::SetPageFlags(nullptr, vaddr2, MemoryFlags::Writable | MemoryFlags::Executable | MemoryFlags::Global);

        ASSERT(res.IsOkayResult()
            , "Failed to apply flags to page %Xp for lock elision: %H"
            , vaddr2, res);
    }

    // msg("B%n");

    //  Finally, no-op the code.

    bool okay = TurnIntoNoOp(reinterpret_cast<void *>(tStart)
        , reinterpret_cast<void *>(tEnd), true);

    ASSERT(okay, "Failed to turn region %Xp-%Xp into a no-op."
        , tStart, tEnd);

    // Debug::DebugTerminal->WriteLine();
    // Debug::DebugTerminal->WriteHexDump(tStart, tEnd - tStart, 16);

    //  And close by restoring the page flags.

    for (uintptr_t i = tStart; i < tEnd; i += 32)
        CpuInstructions::FlushCache((void *)i);

    // msg("D ");

    res = Vmm::SetPageFlags(nullptr, vaddr1, mf1);

    ASSERT(res.IsOkayResult()
        , "Failed to apply flags to page %Xp for lock elision: %H"
        , vaddr1, res);

    if (vaddr2 != vaddr1)
    {
        res = Vmm::SetPageFlags(nullptr, vaddr2, mf2);

        ASSERT(res.IsOkayResult()
            , "Failed to apply flags to page %Xp for lock elision: %H"
            , vaddr2, res);
    }

    // msg("E%n");
}

void TestLockElision()
{
    // msg("Test range 1: %Xp-%Xp%n", testStart1, testEnd1);

    TestFunction1();

    PatchRange(testStart1, testEnd1);

    TestFunction1();

    // msg("Test range 2: %Xp-%Xp%n", testStart2, testEnd2);

    TestFunction2();

    PatchRange(testStart2, testEnd2);

    TestFunction2();
}

#endif
