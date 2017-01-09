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

/**
 *  The `__must_check` attributes are there to make sure that the TicketLockUninterruptible
 *  is not used in place of a normal ticket lock accidentally.
 */

#pragma once

#include <beel/interrupt.state.hpp>

namespace Beelzebub { namespace Synchronization
{
#ifndef __BEELZEBUB_SPINLOCK_T
#define __BEELZEBUB_SPINLOCK_T
    typedef union ticketlock_t
    {
        uint32_t volatile Overall;
        __extension__ struct
        {
            uint16_t volatile Head;
            uint16_t volatile Tail;
        };

        ticketlock_t() = default;
        inline ticketlock_t(uint32_t o) : Overall(o) { }
        inline ticketlock_t(uint16_t h, uint16_t t) : Head(h), Tail(t) { }
    } ticketlock_t;
#endif

    static_assert(sizeof(ticketlock_t) == 4, "");

    //  Lemme clarify here.
    //  For non-SMP builds, SMP ticket locks are gonna be dummies.
    //  For SMP builds, all ticket locks are implemented.

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    /**
     *  Busy-waiting re-entrant synchronization primitive which
     *  prevents CPU interrupts on the locking CPU.
     */
    template<bool SMP>
    struct TicketLockUninterruptible { };

    /**
     *  Busy-waiting re-entrant synchronization primitive which
     *  prevents CPU interrupts on the locking CPU.
     */
    template<>
    struct TicketLockUninterruptible<false>
#else
    /**
     *  Busy-waiting re-entrant synchronization primitive which
     *  prevents CPU interrupts on the locking CPU.
     */
    template<bool SMP>
    struct TicketLockUninterruptible
#endif
    {
    public:

        typedef InterruptState Cookie;

        /*  Constructor(s)  */

        TicketLockUninterruptible() = default;
        TicketLockUninterruptible(TicketLockUninterruptible const &) = delete;
        TicketLockUninterruptible & operator =(TicketLockUninterruptible const &) = delete;
        TicketLockUninterruptible(TicketLockUninterruptible &&) = delete;
        TicketLockUninterruptible & operator =(TicketLockUninterruptible &&) = delete;

        /*  Destructor  */

#ifdef __BEELZEBUB__CONF_DEBUG
        ~TicketLockUninterruptible();
#endif

        /*  Operations  */

#ifdef __BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS
        /**
         *  Acquire the ticket lock, if possible.
         */
        __noinline __must_check bool TryAcquire(Cookie & cookie) volatile;

        /**
         *  Awaits for the ticket lock to be freed.
         *  Does not acquire the lock.
         */
        __noinline void Spin() const volatile;

        /**
         *  Checks if the ticket lock is free. If not, it awaits.
         *  Does not acquire the lock.
         */
        __noinline void Await() const volatile;

        /**
         *  Acquire the ticket lock, waiting if necessary.
         */
        __noinline __must_check Cookie Acquire() volatile;

        /**
         *  Acquire the ticket lock, waiting if necessary.
         */
        __noinline void SimplyAcquire() volatile;

        /**
         *  Release the ticket lock.
         */
        __noinline void Release(Cookie const cookie) volatile;

        /**
         *  Release the ticket lock.
         */
        __noinline void SimplyRelease() volatile;

        /**
         *  Checks whether the ticket lock is free or not.
         */
        __noinline __must_check bool Check() const volatile;
#else
        /**
         *  Acquire the ticket lock, if possible.
         */
        __forceinline __must_check bool TryAcquire(Cookie & cookie) volatile
        {
            cookie = InterruptState::Disable();

            COMPILER_MEMORY_BARRIER();

        op_start:
            uint16_t const oldTail = this->Value.Tail;
            ticketlock_t cmp {oldTail, oldTail};
            ticketlock_t const newVal {oldTail, (uint16_t)(oldTail + 1)};
            ticketlock_t const cmpCpy = cmp;

            asm volatile( "lock cmpxchgl %[newVal], %[curVal] \n\t"
                        : [curVal]"+m"(this->Value), "+a"(cmp)
                        : [newVal]"r"(newVal)
                        : "cc" );

            if likely(cmp.Overall == cmpCpy.Overall)
            {
                cookie.Restore();
                //  If the ticket lock was already locked, restore interrupt state.

                return false;
            }
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;

            return true;
        }

        /**
         *  Awaits for the ticket lock to be freed.
         *  Does not acquire the lock.
         */
        __forceinline void Spin() const volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            ticketlock_t copy;

            do
            {
                copy = {this->Value.Overall};

                asm volatile ( "pause \n\t" );
            } while (copy.Tail != copy.Head);
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_CHK;
        }

        /**
         *  Checks if the ticket lock is free. If not, it awaits.
         *  Does not acquire the lock.
         */
        __forceinline void Await() const volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            ticketlock_t copy = {this->Value.Overall};

            while (copy.Tail != copy.Head)
            {
                asm volatile ( "pause \n\t" );

                copy = {this->Value.Overall};
            }
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_CHK;
        }

        /**
         *  Acquire the ticket lock, waiting if necessary.
         */
        __forceinline __must_check Cookie Acquire() volatile
        {
            Cookie const cookie = InterruptState::Disable();

            COMPILER_MEMORY_BARRIER();

        op_start:
            uint16_t myTicket = 1;

            asm volatile( "lock xaddw %[ticket], %[tail] \n\t"
                        : [tail]"+m"(this->Value.Tail)
                        , [ticket]"+r"(myTicket)
                        : : "cc" );
            //  It's possible to address the upper word directly.

            uint16_t diff;

            while ((diff = myTicket - this->Value.Head) != 0)
                do asm volatile ( "pause \n\t" ); while (--diff != 0);
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;

            return cookie;
        }

        /**
         *  Acquire the ticket lock, waiting if necessary.
         */
        __forceinline void SimplyAcquire() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            uint16_t myTicket = 1;

            asm volatile( "lock xaddw %[ticket], %[tail] \n\t"
                        : [tail]"+m"(this->Value.Tail)
                        , [ticket]"+r"(myTicket)
                        : : "cc" );
            //  It's possible to address the upper word directly.

            uint16_t diff;

            while ((diff = myTicket - this->Value.Head) != 0)
                do asm volatile ( "pause \n\t" ); while (--diff != 0);
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;
        }

        /**
         *  Release the ticket lock.
         */
        __forceinline void Release(Cookie const cookie) volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            asm volatile( "lock addw $1, %[head] \n\t"
                        : [head]"+m"(this->Value.Head)
                        : : "cc" );
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_REL;

            cookie.Restore();
        }

        /**
         *  Release the ticket lock.
         */
        __forceinline void SimplyRelease() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            asm volatile( "lock addw $1, %[head] \n\t"
                        : [head]"+m"(this->Value.Head)
                        : : "cc" );
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_REL;
        }

        /**
         *  Checks whether the ticket lock is free or not.
         */
        __forceinline __must_check bool Check() const volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            ticketlock_t copy = {this->Value.Overall};

            if (copy.Head != copy.Tail)
                return false;
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_CHK;

            return true;
        }
#endif

        /**
         *  Reset the ticket lock.
         */
        __forceinline void Reset() volatile
        {
            this->Value.Overall = 0;
        }

        /*  Fields  */

    private:

        ticketlock_t Value;
    };

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    /**
     *  Busy-waiting re-entrant synchronization primitive which
     *  prevents CPU interrupts on the locking CPU.
     */
    template<>
    struct TicketLockUninterruptible<true>
    {
    public:

        typedef InterruptState Cookie;
        static constexpr Cookie const InvalidCookie = __int_cookie_invalid;

        /*  Constructor(s)  */

        TicketLockUninterruptible() = default;
        TicketLockUninterruptible(TicketLockUninterruptible const &) = delete;
        TicketLockUninterruptible & operator =(TicketLockUninterruptible const &) = delete;
        TicketLockUninterruptible(TicketLockUninterruptible &&) = delete;
        TicketLockUninterruptible & operator =(TicketLockUninterruptible &&) = delete;

        /*  Operations  */

        __forceinline __must_check bool TryAcquire(Cookie & cookie) const volatile
        { cookie = InterruptState::Disable(); return true; }
        __forceinline void Spin() const volatile { }
        __forceinline void Await() const volatile { }
        __forceinline __must_check Cookie Acquire() const volatile
        { return InterruptState::Disable(); }
        __forceinline void SimplyAcquire() const volatile { }

        __forceinline void Release(Cookie const cookie) const volatile
        { cookie.Restore(); }
        __forceinline void SimplyRelease() const volatile { }
        __forceinline __must_check bool Check() const volatile
        { return true; }
    };
#endif
}}
