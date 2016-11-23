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
#include <cores.hpp>
#include <system/interrupt_controllers/lapic.hpp>
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

static __hot bool ExecuteHead()
{
    CpuData * const data = Cpu::GetData();

    MailFunction func = nullptr;
    void * cookie = nullptr;

    withLock (data->MailLock)
    {
        auto head = data->MailHead;

        if unlikely(head == nullptr)
            return false;

        func = head->Function;
        cookie = head->Cookie;

        for (unsigned int i = 0; i < head->DestinationCount; ++i)
        {
            MailboxEntryLink & link = head->Links[i];

            if (link.Core == data->Index)
            {
                data->MailHead = link.Next;
                //  This dequeues the mail entry.

                --head->DestinationsLeft;
                //  This core no longer needs anything from that mail entry.

                break;
            }
        }

        if (data->MailHead == nullptr)
            data->MailTail = nullptr;
    }

    func(cookie);

    return true;
}

static __hot void MailboxIsrHandler(INTERRUPT_HANDLER_ARGS_FULL)
{
    while (ExecuteHead()) { /* loopie loop */ }
    
    END_OF_INTERRUPT();
}

static Spinlock<> InitLock {};
static bool Initialized = false;

ObjectAllocatorSmp SharedQueue;

/******************************
    MailboxEntryBase struct
******************************/

/*  Operations  */

void MailboxEntryBase::Post(bool poll)
{
    return Mailbox::Post(this, poll);
}

/********************
    Mailbox class
********************/

/*  Initialization  */

void Mailbox::Initialize()
{
    //  A lock is used here because this code must only be executed once, and
    //  other cores should wait for it to finish.

    withLock (InitLock)
    {
        if (Initialized)
            return;

        auto const vec = Interrupts::Get(KnownExceptionVectors::Mailbox);
        //  Unique.

        vec.SetHandler(&MailboxIsrHandler);
        vec.SetEnder(&Lapic::IrqEnder);

        new (&SharedQueue) ObjectAllocatorSmp(sizeof(MailboxEntry<8>), __alignof(MailboxEntry<8>)
            , &AcquirePoolInKernelHeap, &EnlargePoolInKernelHeap, &ReleasePoolFromKernelHeap);

        Initialized = true;
    }
}

/*  Operation  */

void Mailbox::Post(MailboxEntryBase * entry, bool poll)
{
    assert(entry != nullptr);

    InterruptGuard<> intGuard;
    //  Everything *has* to be done under a lock guard.

    if (entry->DestinationCount == 1 && entry->Links[0].Core == Broadcast)
    {
        auto tgCnt = Cores::GetCount() - 1;     //  Target count.
        auto thisCore = Cpu::GetData()->Index;  //  Index of this core.

        ALLOCATE_MAIL_4(newEntry, tgCnt, entry->Function, entry->Cookie);

        for (unsigned int link = 0; link < tgCnt; ++link)
            newEntry.Links[link] = MailboxEntryLink((link < thisCore) ? link : (link + 1));
        //  Set up all the links properly.

        return PostInternal(&newEntry, poll, true);
    }
    else
        return PostInternal(entry, poll, false);
}

void Mailbox::PostInternal(MailboxEntryBase * entry, bool poll, bool broadcast)
{
    for (unsigned int i = 0; i < entry->DestinationCount; ++i)
    {
        uint32_t const targetCore = entry->Links[i].Core;
        CpuData * const target = Cores::Get(targetCore);

        withLock (target->MailLock)
        {
            if likely(target->MailTail != nullptr)
            {
                //  Mail is already queued for this core, so the entry is added to the tail.

                MailboxEntryBase * const tail = target->MailTail;

                for (unsigned int j = 0; j < entry->DestinationCount; ++j)
                {
                    MailboxEntryLink & link = tail->Links[j];

                    if (link.Core == targetCore)
                    {
                        link.Next = entry;

                        break;
                    }
                }

                target->MailTail = entry;
            }
            else
                target->MailTail = target->MailHead = entry;    //  ezpz
        }

        if unlikely(!broadcast)
            Lapic::SendIpi(LapicIcr(0)
                .SetDeliveryMode(InterruptDeliveryModes::Fixed)
                .SetDestinationShorthand(IcrDestinationShorthand::None)
                .SetAssert(true)
                .SetDestination(target->LapicId)
                .SetVector(Interrupts::Get(KnownExceptionVectors::Mailbox).GetVector()));
    }

    if likely(broadcast)
        Lapic::SendIpi(LapicIcr(0)
            .SetDeliveryMode(InterruptDeliveryModes::Fixed)
            .SetDestinationShorthand(IcrDestinationShorthand::AllExcludingSelf)
            .SetAssert(true)
            .SetVector(Interrupts::Get(KnownExceptionVectors::Mailbox).GetVector()));

    while (entry->DestinationsLeft > 0)
        if (!(poll && ExecuteHead()))
            CpuInstructions::DoNothing();
}
