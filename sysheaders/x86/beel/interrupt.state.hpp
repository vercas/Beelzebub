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

#pragma once

#include <beel/metaprogramming.h>

namespace Beelzebub
{
    /// <summary>Guards a scope from interrupts.</summary>
    template<bool en = false>
    struct InterruptGuard;

    /****************************
        InterruptState struct
    ****************************/

    struct InterruptState
    {
        template<bool en>
        friend struct InterruptGuard;

        /*  Statics  */

        static constexpr void const * const InvalidValue = nullptr;

        static inline bool IsEnabled() __must_check
        {
            uintptr_t flags;

            asm volatile("pushf        \n\t"
                         "pop %[flags] \n\t"
                        : [flags]"=r"(flags));
            //  Push and pop don't change any flags. Yay!

            return (flags & ((uintptr_t)1 << 9)) != 0;
        }

    private:
        static inline void const * DisableInner() __returns_nonnull __malloc
        {
            void const * cookie;

            asm volatile("pushf      \n\t"
                         "cli        \n\t"
                         "pop %[dst] \n\t"
                        : [dst]"=r"(cookie)
                        :
                        : "memory");

            /*  A bit of wisdom from froggey: ``On second thought, the constraint for flags ["cookie"] in
                interrupt_disable [PushDisableInterrupts] should be "=r", not "=rm". If the compiler decided
                to store [the] flags on the stack and generated an (E|R)SP-relative address, the address would
                end up being off by 4/8 [when passed onto the pop instruction because the stack pointer changed].'' */

            return cookie;
        }

    public:
        /**
         *  <summary>
         *  Disables interrupts and returns a cookie which allows restoring the
         *  interrupt state as it was before executing this function.
         *  </summary>
         *  <return>
         *  A cookie which allows restoring the interrupt state as it was before
         *  executing this function.
         *  </return>
         */
        static inline InterruptState Disable() __must_check
        {
            return InterruptState(DisableInner());
        }

    private:
        static inline void const * EnableInner() __returns_nonnull __malloc
        {
            void const * cookie;

            asm volatile("pushf      \n\t"
                         "sti        \n\t"
                         "pop %[dst] \n\t"
                        : [dst]"=r"(cookie)
                        :
                        : "memory");

            return cookie;
        }

    public:
        /**
         *  <summary>
         *  Enables interrupts and returns a cookie which allows restoring the
         *  interrupt state as it was before executing this function.
         *  </summary>
         *  <return>
         *  A cookie which allows restoring the interrupt state as it was before
         *  executing this function.
         *  </return>
         */
        static inline InterruptState Enable() __must_check
        {
            return InterruptState(EnableInner());
        }

        /*  Constructors  */

        __forceinline InterruptState() : Value() { }

    private:
        __forceinline explicit InterruptState(void const * val) : Value( val) { }

    public:
        __forceinline InterruptState(InterruptState const & other) : Value(other.Value) { }
        __forceinline InterruptState(InterruptState && other) : Value(other.Value) { }

        InterruptState & operator =(InterruptState const & other) { this->Value = other.Value; return *this; }
        InterruptState & operator =(InterruptState && other) { this->Value = other.Value; return *this; }

        /*  Operations  */

        __forceinline void Restore() const
        {
            asm volatile("push %[src] \n\t"   //  PUT THE COOKIE DOWN!
                         "popf        \n\t"
                        :
                        : [src]"rm"(this->Value)
                        : "memory", "cc");

            //  Here the cookie can safely be retrieved from the stack because
            //  RSP will change after push, not before.
        }

        __forceinline bool GetEnabled() const
        {
            return ((uintptr_t)this->Value & ((uintptr_t)1 << 9)) != 0;
        }

        /*  Fields  */

    private:
        void const * Value;
    };

    /***********************
        Interrupt Guards
    ***********************/

    /// <summary>Guards a scope by disabling interrupts.</summary>
    template<>
    struct InterruptGuard<false>
    {
        /*  Constructor(s)  */

        inline InterruptGuard() : Cookie(InterruptState::DisableInner()) { }

        InterruptGuard(InterruptGuard const &) = delete;
        InterruptGuard(InterruptGuard && other) = delete;
        InterruptGuard & operator =(InterruptGuard const &) = delete;
        InterruptGuard & operator =(InterruptGuard &&) = delete;

        /*  Destructor  */

        inline ~InterruptGuard()
        {
            this->Cookie.Restore();
        }

    private:
        /*  Field(s)  */

        InterruptState const Cookie;
    };

    /// <summary>Guards a scope by enabling interrupts.</summary>
    template<>
    struct InterruptGuard<true>
    {
        /*  Constructor(s)  */

        inline InterruptGuard() : Cookie(InterruptState::EnableInner()) { }

        InterruptGuard(InterruptGuard const &) = delete;
        InterruptGuard(InterruptGuard && other) = delete;
        InterruptGuard & operator =(InterruptGuard const &) = delete;
        InterruptGuard & operator =(InterruptGuard &&) = delete;

        /*  Destructor  */

        inline ~InterruptGuard()
        {
            this->Cookie.Restore();
        }

    private:
        /*  Field(s)  */

        InterruptState const Cookie;
    };

    #define withInterrupts(val) with(Beelzebub::InterruptGuard<val> MCATS(_int_guard, __LINE__))
}
