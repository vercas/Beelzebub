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

#include <mailbox.hpp>
#include <system/interrupt_controllers/lapic.hpp>
#include <system/cpu.hpp>
#include <synchronization/spinlock.hpp>
#include <memory/object_allocator_smp.hpp>
#include <memory/object_allocator_pools_heap.hpp>
#include <kernel.hpp>
#include <string.h>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;

/****************
    Internals
****************/

struct QueueItem
{
    MailboxEntryBase * Entry;
};

static __cold void MailboxIsrHandler(INTERRUPT_HANDLER_ARGS_FULL)
{
    CpuData * const data = Cpu::GetData();

    
    END_OF_INTERRUPT();
}

static Spinlock<> InitLock {};
static bool Initialized = false;

ObjectAllocatorSmp SharedQueue;

/********************
    Mailbox class
********************/

/*  Initialization  */

void Mailbox::Initialize()
{
    auto const vec = Interrupts::Get(KnownExceptionVectors::Mailbox);
    //  Unique.

    CpuData * const data = Cpu::GetData();

    //  A lock is used here because this code must only be executed once, and
    //  other cores should wait for it to finish.

    withLock (InitLock)
    {
        if (Initialized)
            return;

        vec.SetHandler(&MailboxIsrHandler);
        vec.SetEnder(&Lapic::IrqEnder);

        new (&SharedQueue) ObjectAllocatorSmp(sizeof(MailboxEntry<8>), __alignof(MailboxEntry<8>)
            , &AcquirePoolInKernelHeap, &EnlargePoolInKernelHeap, &ReleasePoolFromKernelHeap);

        Initialized = true;
    }
}

/*  Operation  */

void Mailbox::PostInternal(MailboxEntryBase * entry, unsigned int N, bool poll)
{

}
