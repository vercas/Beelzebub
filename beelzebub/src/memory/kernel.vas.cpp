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
#include <beel/interrupt.state.hpp>
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

/*  Constructors  */

KernelVas::KernelVas()
    : Vas()
    , EnlargingCore(SpecialNoCore)
    , Bootstrapping(true)
    // , PreallocatedDescriptors()
{

}

/*  Support  */

// Handle KernelVas::AllocateNode(AvlTree<MemoryRegion>::Node * & node)
// {
//     return this->Alloc.AllocateObject(node);
// }

// Handle KernelVas::RemoveNode(AvlTree<MemoryRegion>::Node * const node)
// {
//     return this->Alloc.DeallocateObject(node);
// }

Handle KernelVas::PreOp(bool & lock, bool alloc)
{
    (void)alloc;

    if unlikely(this->EnlargingCore == (likely(Cores::IsReady()) ? Cpu::GetData()->Index : Cpu::ComputeIndex()))
    {
        MSG_("Core %us re-entered to enlarge the KVAS.%n", this->EnlargingCore);

        lock = false;
    }

    return HandleResult::Okay;
}

Handle KernelVas::PostOp(Handle oRes, bool lock, bool alloc)
{
    (void)lock;
    (void)alloc;

    if likely(this->Alloc.GetFreeCount() >= FreeDescriptorsThreshold)
        return oRes;
    else if unlikely(this->EnlargingCore == SpecialNoCore)
    {
        //  Gotta enlarge.

        this->EnlargingCore = likely(Cores::IsReady()) ? Cpu::GetData()->Index : Cpu::ComputeIndex();

        MSG_("Core %us is enlarging the KVAS.%n", this->EnlargingCore);

        Handle res = this->Alloc.ForceExpand(FreeDescriptorsThreshold - this->Alloc.GetFreeCount());

        MSG_("Core %us finished enlarging the KVAS: %H%n", this->EnlargingCore, res);

        this->EnlargingCore = SpecialNoCore;

        return oRes == HandleResult::Okay ? res : oRes;
    }

    return oRes;
}
