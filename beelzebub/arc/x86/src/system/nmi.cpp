/*
    Copyright (c) 2017 Alexandru-Mihai Maftei. All rights reserved.


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

#include "system/nmi.hpp"
#include "system/interrupt_controllers/lapic.hpp"

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;

/****************
    Nmi class
****************/

/*  Statics  */

Nmi::HandlerNode * Nmi::Handlers = nullptr;
Nmi::EnderNode * Nmi::Enders = nullptr;

/*  Initialization  */

void Nmi::Initialize()
{
    Interrupts::Get(KnownExceptionVectors::NmiInterrupt).SetHandler(&Handler);
}

void Nmi::AddHandler(Nmi::HandlerNode * e)
{
    ASSERT(e->Function != nullptr);

    HandlerNode * * next = &Handlers;

    while (*next != nullptr)
        next = &((*next)->Next);

    *next = e;

    ASSERT(e->Next == nullptr);
}

void Nmi::AddEnder(Nmi::EnderNode * e, bool unique)
{
    EnderNode * * next = &Enders;

    while (*next != nullptr)
    {
        if unlikely(unique && (*next)->Function == e->Function)
            return;

        next = &((*next)->Next);
    }

    *next = e;
}

/*  Operation  */

void Nmi::Broadcast()
{
    Lapic::SendIpi(LapicIcr(0)
        .SetDeliveryMode(InterruptDeliveryModes::NMI)
        .SetDestinationShorthand(IcrDestinationShorthand::AllExcludingSelf)
        .SetAssert(true));
}

void Nmi::Send(uint32_t id)
{
    Lapic::SendIpi(LapicIcr(0)
        .SetDeliveryMode(InterruptDeliveryModes::NMI)
        .SetDestinationShorthand(IcrDestinationShorthand::None)
        .SetAssert(true)
        .SetDestination(id));
}

void Nmi::Handler(INTERRUPT_HANDLER_ARGS_FULL)
{
    for (HandlerNode * han = Handlers; han != nullptr; han = han->Next)
    {
        ASSERT(han->Function != nullptr);

        han->Function(state, nullptr, handler, vector);
    }

    for (EnderNode * end = Enders; end != nullptr; end = end->Next)
        end->Function(handler, vector);

    END_OF_INTERRUPT();
}
