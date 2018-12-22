/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

#include "irqs.hpp"
#include "system/interrupts.hpp"
#include "system/exceptions.hpp"
#include "system/nmi.hpp"
#include "system/gdt.hpp"
#include "system/interrupt_controllers/pic.hpp"
#include <beel/sync/smp.lock.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;

namespace Beelzebub
{
    bool operator == (isr_t a, irq_t b)
    {
        return a.Value == b.Value + IrqsOffset;
    }
}

struct InterruptVectorData
{
    SmpLock CS;
    InterruptHandlerNode * First;
    InterruptEnderNode const * Ender;
} Vectors[Interrupts::Count];

/**********************************
    InterruptHandlerNode struct
**********************************/

/*  Operation  */

IrqSubscribeResult InterruptHandlerNode::Subscribe(isr_t isr)
{
    if unlikely(isr.Value < 0 || isr.Value >= (isr_inner_t)Interrupts::Count)
        return IrqSubscribeResult::VectorOutOfRange;

    if unlikely(this->IsSubscribed())
        return IrqSubscribeResult::AlreadySubscribed;

    InterruptVectorData * const vd = Vectors + isr.Value;
    bool added;

    withLock (vd->CS)
    {
        added = this->AddToList(&(vd->First));

        if likely(added)
        {
            this->IsrVector = isr;
        }
    }

    return added ? IrqSubscribeResult::Success : IrqSubscribeResult::Unuseable;
}

IrqSubscribeResult InterruptHandlerNode::Subscribe(irq_t irq)
{
    if unlikely(irq.Value < 0 || irq.Value >= 48)
        return IrqSubscribeResult::VectorOutOfRange;
    //  TODO: proper limits.

    auto res = this->Subscribe(isr_t(irq.Value + IrqsOffset));

    if likely(res == IrqSubscribeResult::Success)
        this->IrqVector = irq;

    return res;
}

IrqUnsubscribeResult InterruptHandlerNode::Unsubscribe()
{
    //ASSERTX(this->IsrVector >= 0 && this->IsrVector < Interrupts::Count)(this->IsrVector)XEND

    if (this->IsrVector.Value < 0 || this->IsrVector.Value >= (isr_inner_t)Interrupts::Count)
        return IrqUnsubscribeResult::NotSubscribed;

    InterruptVectorData * const vd = Vectors + this->IsrVector.Value;
    bool removed;

    withLock (vd->CS)
    {
        removed = this->RemoveFromList(&(vd->First));

        if likely(removed)
        {
            this->IsrVector = isr_invalid;
            this->IrqVector = irq_invalid;
        }
    }

    ASSERT(removed);

    return IrqUnsubscribeResult::Success;
}

/********************************
    InterruptEnderNode struct
********************************/

/*  Statics  */

InterruptEnderNode const * InterruptEnderNode::Get(isr_t isr)
{
    if unlikely(isr.Value < 0 || isr.Value >= (isr_inner_t)Interrupts::Count)
        return nullptr;

    return Vectors[isr.Value].Ender;
}

InterruptEnderNode const * InterruptEnderNode::Get(irq_t irq)
{
    if unlikely(irq.Value < 0 || irq.Value >= 48)
        return nullptr;
    //  TODO: Magic value.

    return Get(isr_t(irq.Value + IrqsOffset));
}

/*  Operations  */

IrqEnderRegisterResult InterruptEnderNode::Register(isr_t const isr) const
{
    if unlikely(isr.Value < 0 || isr.Value >= (isr_inner_t)Interrupts::Count)
        return IrqEnderRegisterResult::VectorOutOfRange;

    InterruptVectorData * const vd = Vectors + isr.Value;

    if unlikely(vd->Ender != nullptr)
        return IrqEnderRegisterResult::AlreadyRegistered;

    vd->Ender = this;

    return IrqEnderRegisterResult::Success;
}

IrqEnderRegisterResult InterruptEnderNode::Register(irq_t const irq) const
{
    if unlikely(irq.Value < 0 || irq.Value >= 48)
        return IrqEnderRegisterResult::VectorOutOfRange;
    //  TODO: Magic value.

    return this->Register(isr_t(irq.Value + IrqsOffset));
}

IrqEnderUnregisterResult InterruptEnderNode::Unregister(isr_t const isr) const
{
    if unlikely(isr.Value < 0 || isr.Value >= (isr_inner_t)Interrupts::Count)
        return IrqEnderUnregisterResult::VectorOutOfRange;

    InterruptVectorData * const vd = Vectors + isr.Value;

    if unlikely(vd->Ender == nullptr)
        return IrqEnderUnregisterResult::NotRegistered;
    if unlikely(vd->Ender != this)
        return IrqEnderUnregisterResult::WrongEnder;

    vd->Ender = nullptr;

    return IrqEnderUnregisterResult::Success;
}

IrqEnderUnregisterResult InterruptEnderNode::Unregister(irq_t const irq) const
{
    if unlikely(irq.Value < 0 || irq.Value >= 48)
        return IrqEnderUnregisterResult::VectorOutOfRange;
    //  TODO: Magic value.

    return this->Unregister(isr_t(irq.Value + IrqsOffset));
}

bool InterruptEnderNode::IsRegistered(isr_t const isr) const
{
    if unlikely(isr.Value < 0 || isr.Value >= (isr_inner_t)Interrupts::Count)
        return false;

    return Vectors[isr.Value].Ender == this;
}

bool InterruptEnderNode::IsRegistered(irq_t const irq) const
{
    if unlikely(irq.Value < 0 || irq.Value >= 48)
        return false;
    //  TODO: Magic value.

    return this->IsRegistered(isr_t(irq.Value + IrqsOffset));
}

/*****************
    Irqs class
*****************/

/*  Constants  */

//  These are all calculated so that one can freely do +-1,000,000 around them.
size_t const Irqs::MaxPriority;
size_t const Irqs::VeryHighPriority;
size_t const Irqs::HighPriority;
size_t const Irqs::MediumPriority;
size_t const Irqs::LowPriority;
size_t const Irqs::VeryLowPriority;
size_t const Irqs::MinPriority;

#define EISR(name, val) constexpr isr_t const Irqs::name;
#define EIRQ(name, val) constexpr irq_t const Irqs::name;

#ifdef ENUM_KNOWNISRS
    ENUM_KNOWNISRS(EISR)
#endif

#ifdef ENUM_KNOWNIRQS
    ENUM_KNOWNIRQS(EIRQ)
#endif

static bool Ready = false;

/*  Initialization  */

Handle Irqs::Initialize()
{
    ASSERT(!Ready);

    for (size_t i = 0; i < Interrupts::Count; ++i)
    {
        Interrupts::Get(i)
        .SetGate(
            IdtGate()
            .SetOffset(&IsrStubsBegin + i)
            .SetSegment(Gdt::KernelCodeSegment)
            .SetType(IdtGateType::InterruptGate)
            .SetPresent(true))
        .SetHandler(&CommonInterruptHandler);
    }

    //  So now the IDT should be fresh out of the oven and ready for serving.

#if   defined(__BEELZEBUB__ARCH_AMD64)
    Interrupts::Get(KnownIsrs::DoubleFault).GetGate()->SetIst(1);
    Interrupts::Get(KnownIsrs::PageFault  ).GetGate()->SetIst(2);
#endif

    Interrupts::Get(KnownIsrs::DivideError).SetHandler(&DivideErrorHandler);
    Interrupts::Get(KnownIsrs::Breakpoint).SetHandler(&BreakpointHandler);
    Interrupts::Get(KnownIsrs::Overflow).SetHandler(&OverflowHandler);
    Interrupts::Get(KnownIsrs::BoundRangeExceeded).SetHandler(&BoundRangeExceededHandler);
    Interrupts::Get(KnownIsrs::InvalidOpcode).SetHandler(&InvalidOpcodeHandler);
    Interrupts::Get(KnownIsrs::NoMathCoprocessor).SetHandler(&NoMathCoprocessorHandler);
    Interrupts::Get(KnownIsrs::DoubleFault).SetHandler(&DoubleFaultHandler);
    Interrupts::Get(KnownIsrs::InvalidTss).SetHandler(&InvalidTssHandler);
    Interrupts::Get(KnownIsrs::SegmentNotPresent).SetHandler(&SegmentNotPresentHandler);
    Interrupts::Get(KnownIsrs::StackSegmentFault).SetHandler(&StackSegmentFaultHandler);
    Interrupts::Get(KnownIsrs::GeneralProtectionFault).SetHandler(&GeneralProtectionHandler);
    Interrupts::Get(KnownIsrs::PageFault).SetHandler(&PageFaultHandler);

    Pic::Initialize(IrqsOffset);

    Pic::SetMasked(1, false);
    Pic::SetMasked(3, false);
    Pic::SetMasked(4, false);
    // Pic::Subscribe(1, &keyboard_handler);
    // Pic::Subscribe(3, &ManagedSerialPort::IrqHandler);
    // Pic::Subscribe(4, &ManagedSerialPort::IrqHandler);

    Interrupts::Register.Activate();

    Ready = true;

    return HandleResult::Okay;
}

bool Irqs::AreReady()
{
    return Ready;
}

/*  Handler  */

struct InterruptContextInternal : InterruptContext
{
    /*  Constructors  */

    inline constexpr InterruptContextInternal(InterruptStackState * reg, isr_t isr)
        : InterruptContext(reg, isr, irq_invalid)
    {
        if (isr.Value >= IrqsOffset && isr.Value < (IrqsOffset + 48)) //  TODO: Magic limit.
            this->IrqVector = irq_t(isr.Value - IrqsOffset);
    }
};

void Irqs::CommonInterruptHandler(INTERRUPT_HANDLER_ARGS)
{
    (void)handler;

    InterruptVectorData const * const vd = Vectors + vector;
    
    InterruptContextInternal context { state, isr_t(vector) };

    context.CurrentHandler = vd->First;

    //  TODO: Exception trapping?

    while (context.CurrentHandler != nullptr)
    {
        context.CurrentHandler->Handler(&context, context.CurrentHandler->Cookie);

        ++context.Counter;
        context.CurrentHandler = context.CurrentHandler->Next;
    }

    if (vd->Ender)
        vd->Ender->Ender(&context, vd->Ender->Cookie, InterruptEndType::AfterKernel);
    //  TODO: Userland-handled interrupts.
}
