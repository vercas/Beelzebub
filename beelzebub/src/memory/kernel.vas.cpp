/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


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

#include <memory/kernel.vas.hpp>
#include <system/interrupts.hpp>
#include <cores.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::Utils;

/**********************
    KernelVas class
**********************/

/*  Support  */

bool KernelVas::PreCheck(bool & lock, bool alloc)
{
    if unlikely(lock && this->SpecialAllocationLocker != SpecialLockFree)
    {
        uint32_t const ind = likely(Cores::IsReady()) ? Cpu::GetData()->Index : Cpu::ComputeIndex();

        if unlikely(this->SpecialAllocationLocker == ind)
        {
            //  If the current core is holding the special lock, no need to re-lock the VAS.
            //  This is not subject to race conditions because the locking core is uninterruptible.

            assert(alloc, "The core re-entering the pre-check should be doing an allocation.");

            lock = false;
        }

        return false;
        //  No matter which core locked, the post-check need not run again.
    }

    return true;
}

Handle KernelVas::PostCheck()
{
    //  Keep in mind this is running under the VAS lock and uninterruptible.

    if (this->Alloc.GetFreeCount() < FreeDescriptorsThreshold)
    {
        uint32_t old = SpecialLockFree;
        uint32_t const ind = likely(Cores::IsReady()) ? Cpu::GetData()->Index : Cpu::ComputeIndex();

        if (this->SpecialAllocationLocker.CmpXchgStrong(old, ind))
        {
            //  This core acquired the lock.

            MSG("Core %u4 is about to expand the allocator for KVAS descriptors!%n", ind);

            Handle res = this->Alloc.ForceExpand(FreeDescriptorsThreshold - this->Alloc.GetFreeCount());

            this->SpecialAllocationLocker.Store(SpecialLockFree);

            return res;
        }
        else
            while (this->SpecialAllocationLocker.Load() != SpecialLockFree)
                CpuInstructions::DoNothing();
    }

    return HandleResult::Okay;
}
