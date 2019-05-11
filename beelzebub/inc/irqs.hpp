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

/*  Note that the implementation of this header is architecture-specific.  */

#pragma once

#include "system/interrupts.hpp"
#include <beel/structs.kernel.h>

namespace Beelzebub
{
    class Irqs;
    struct InterruptHandlerNode;
    struct InterruptEnderNode;

    using isr_inner_t = int;
    using irq_inner_t = int;

    union isr_t
    {
        isr_inner_t Value;

        __artificial explicit constexpr isr_t() : Value((isr_inner_t)(-1L)) { }
        __artificial explicit constexpr isr_t(isr_inner_t val) : Value(val) { }

        __artificial explicit operator isr_inner_t() const { return this->Value; }
    };

    union irq_t
    {
        irq_inner_t Value;

        __artificial explicit constexpr irq_t() : Value((irq_inner_t)(-1L)) { }
        __artificial explicit constexpr irq_t(irq_inner_t val) : Value(val) { }

        __artificial explicit operator irq_inner_t() const { return this->Value; }
    };

    constexpr isr_t const isr_invalid {};
    constexpr irq_t const irq_invalid {};

    #define CMP_OP(TYP, OP) __artificial constexpr bool operator OP (TYP a, TYP b) { return a.Value OP b.Value; }
    CMP_OP(isr_t, ==) CMP_OP(isr_t, !=) CMP_OP(irq_t, ==) CMP_OP(irq_t, !=)
    #undef CMP_OP

    bool operator == (isr_t a, irq_t b);    //  Note: might as well return false all the time if it applies to the platform.
    __artificial bool operator == (irq_t a, isr_t b) { return b == a; }
    __artificial bool operator != (isr_t a, irq_t b) { return !(a == b); }
    __artificial bool operator != (irq_t a, isr_t b) { return !(b == a); }

    struct InterruptContext
    {
        /*  Constructors  */

        inline constexpr InterruptContext(System::InterruptStackState * reg, isr_t isr, irq_t irq, InterruptContext * next)
            : Registers( reg)
            , IsrVector(isr)
            , IrqVector(irq)
            , CurrentHandler(nullptr)
            , Counter(0)
            , ValidCounter(0)
            , Next(next)
        {

        }

        /*  Fields  */

        System::InterruptStackState * Registers;
        isr_t IsrVector;
        irq_t IrqVector;

        InterruptHandlerNode * CurrentHandler;
        size_t Counter, ValidCounter;

        InterruptContext * Next;
    };

    typedef void (* InterruptHandlerVoid)(InterruptContext const * context, void * cookie);

    template<typename TCookie>
    using InterruptHandler = void (*)(InterruptContext const *, TCookie *);

    enum class InterruptEndType
    {
        AfterKernel,
        BeforeUserland,
        AfterUserland,
    };

    typedef void (* InterruptEnderVoid)(InterruptContext const * context, void * cookie, InterruptEndType type);

    template<typename TCookie>
    using InterruptEnder = void (*)(InterruptContext const *, TCookie *, InterruptEndType);

    enum class IrqSubscribeResult
    {
        Success, VectorOutOfRange, AlreadySubscribed, Unuseable, OutOfMemory,
    };

    enum class IrqUnsubscribeResult
    {
        Success, NotSubscribed,
    };

    struct InterruptHandlerNode
    {
        friend class Irqs;

        /*  Constructors  */

        inline constexpr InterruptHandlerNode(InterruptHandlerVoid han, void * cookie = nullptr, size_t priority = SIZE_MAX / 2)
            : Handler(han)
            , Cookie(cookie)
            , Priority(priority)
            , Next(nullptr)
            , IsrVector(), IrqVector()
        {

        }

        template<typename TCookie>
        inline constexpr InterruptHandlerNode(InterruptHandler<TCookie> han, TCookie * cookie, size_t priority = SIZE_MAX / 2)
            : InterruptHandlerNode(static_cast<InterruptHandlerVoid>(han), static_cast<void *>(cookie), priority)
        {

        }

        /*  Fields  */

        InterruptHandlerVoid const Handler;
        void * const Cookie;
        size_t const Priority;

        /*  Properties  */

        inline isr_t GetIsrVector() const { return this->IsrVector; }
        inline irq_t GetIrqVector() const { return this->IrqVector; }

        inline bool IsSubscribed() const { return this->IsrVector != isr_invalid || this->IrqVector != irq_invalid; }
        inline bool IsLinked() const { return this->Next != nullptr; }

        /*  Operations  */

        IrqSubscribeResult Subscribe(isr_t isr);
        IrqSubscribeResult Subscribe(irq_t irq);

        IrqUnsubscribeResult Unsubscribe();

    private:
        /*  Fields  */

        InterruptHandlerNode * Next;
        isr_t IsrVector;
        irq_t IrqVector;

        /*  Linkage  */

        bool AddToList(InterruptHandlerNode * * first);
        bool RemoveFromList(InterruptHandlerNode * * first);
    };

    enum class IrqEnderRegisterResult
    {
        Success, VectorOutOfRange, AlreadyRegistered,
    };

    enum class IrqEnderUnregisterResult
    {
        Success, VectorOutOfRange, NotRegistered, WrongEnder,
    };

    struct InterruptEnderNode
    {
        friend class Irqs;

        /*  Statics  */

        static InterruptEnderNode const * Get(isr_t isr);
        static InterruptEnderNode const * Get(irq_t irq);

        /*  Constructors  */

        inline constexpr InterruptEnderNode(InterruptEnderVoid end, void * cookie = nullptr)
            : Ender(end)
            , Cookie(cookie)
        {

        }

        template<typename TCookie>
        inline constexpr InterruptEnderNode(InterruptEnder<TCookie> end, TCookie * cookie)
            : InterruptEnderNode(static_cast<InterruptEnderVoid>(end), static_cast<void *>(cookie))
        {

        }

        /*  Fields  */

        InterruptEnderVoid const Ender;
        void * const Cookie;

        /*  Operations  */

        IrqEnderRegisterResult Register(isr_t isr) const;
        IrqEnderRegisterResult Register(irq_t irq) const;

        IrqEnderUnregisterResult Unregister(isr_t isr) const;
        IrqEnderUnregisterResult Unregister(irq_t irq) const;

        bool IsRegistered(isr_t isr) const;
        bool IsRegistered(irq_t irq) const;
    };

    /**
     *  <summary>Represents an abstraction of the system's IRQs and ISRs.</summary>
     */
    class Irqs
    {
    public:
        /*  Constants  */

        //  These are all calculated so that one can freely do +-1,000,000 around them.
        static constexpr size_t const MaxPriority      = SIZE_MAX / 2 + SIZE_MAX / 4 + SIZE_MAX / 8 + SIZE_MAX / 16;
        static constexpr size_t const VeryHighPriority = SIZE_MAX / 2 + SIZE_MAX / 4 + SIZE_MAX / 8;
        static constexpr size_t const HighPriority     = SIZE_MAX / 2 + SIZE_MAX / 4;
        static constexpr size_t const MediumPriority   = SIZE_MAX / 2;
        static constexpr size_t const LowPriority      = SIZE_MAX / 2 - SIZE_MAX / 4;
        static constexpr size_t const VeryLowPriority  = SIZE_MAX / 2 - SIZE_MAX / 4 - SIZE_MAX / 8;
        static constexpr size_t const MinPriority      = SIZE_MAX / 2 - SIZE_MAX / 4 - SIZE_MAX / 8 - SIZE_MAX / 16;

#ifdef ENUM_KNOWNISRS
    #define EISR(name, val) static constexpr isr_t const name { val };

        ENUM_KNOWNISRS(EISR)

    #undef EISR
#endif

#ifdef ENUM_KNOWNIRQS
    #define EIRQ(name, val) static constexpr irq_t const name { val };

        ENUM_KNOWNIRQS(EIRQ)

    #undef EIRQ
#endif

        /*  Statics  */

        static __thread InterruptContext * CurrentContext;

    protected:
        /*  Constructor(s)  */

        Irqs() = default;

    public:
        Irqs(Irqs const &) = delete;
        Irqs & operator =(Irqs const &) = delete;

        /*  Initialization  */

        static __startup Handle Initialize();

        static bool AreReady();

        /*  Handler  */

        static __hot __solid void CommonInterruptHandler(INTERRUPT_HANDLER_ARGS);
    };
}
