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

#pragma once

#include <beel/metaprogramming.h>

namespace Beelzebub { namespace Synchronization
{
#ifndef __BEELZEBUB_TICKETLOCK_CXX_T
#define __BEELZEBUB_TICKETLOCK_CXX_T
    typedef union ticketlock_t
    {
        uint32_t Overall;

        __extension__ struct
        {
            uint16_t Tail, Head;
        };

        ticketlock_t() = default;
        inline ticketlock_t(uint32_t o) : Overall(o) { }
        inline ticketlock_t(uint16_t t, uint16_t h) : Tail(t), Head(h) { }
    } ticketlock_t;
#endif

    static_assert(sizeof(ticketlock_t) == 4, "");

    //  Lemme clarify here.
    //  For non-SMP builds, SMP ticket locks are gonna be dummies.
    //  For SMP builds, all ticket locks are implemented.

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    /**
     *  Busy-waiting re-entrant synchronization primitive.
     */
    template<bool SMP>
    struct TicketLock { };

    /**
     *  Busy-waiting re-entrant synchronization primitive.
     */
    template<>
    struct TicketLock<false>
#else
    /**
     *  Busy-waiting re-entrant synchronization primitive.
     */
    template<bool SMP>
    struct TicketLock
#endif
    {
    public:

        typedef void Cookie;

        /*  Constructor(s)  */

        TicketLock() = default;
        TicketLock(TicketLock const &) = delete;
        TicketLock & operator =(TicketLock const &) = delete;
        TicketLock(TicketLock &&) = delete;
        TicketLock & operator =(TicketLock &&) = delete;

        /*  Destructor  */

#ifdef __BEELZEBUB__CONF_DEBUG
        ~TicketLock();
#endif

        /*  Operations  */

#ifdef __BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS
        /**
         *  Acquire the ticket lock, if possible.
         */
        __solid __must_check bool TryAcquire() volatile;

        /**
         *  Awaits for the ticket lock to be freed.
         *  Does not acquire the lock.
         */
        __solid void Spin() const volatile;

        /**
         *  Checks if the ticket lock is free. If not, it awaits.
         *  Does not acquire the lock.
         */
        __solid void Await() const volatile;

        /**
         *  Acquire the ticket lock, waiting if necessary.
         */
        __solid void Acquire() volatile;

        /**
         *  Release the ticket lock.
         */
        __solid void Release() volatile;

        /**
         *  Checks whether the ticket lock is free or not.
         */
        __solid __must_check bool Check() const volatile;

#else

        /**
         *  Acquire the ticket lock, if possible.
         */
        __forceinline __must_check bool TryAcquire() volatile
        {
            COMPILER_MEMORY_BARRIER();
            
        op_start:
            uint16_t const oldHead = this->Value.Head;

            FORCE_EVAL(oldHead);

            ticketlock_t cmp {oldHead, oldHead};
            ticketlock_t const newVal {oldHead, (uint16_t)(oldHead + 1)};

            if (!__atomic_compare_exchange_n(&(this->Value.Overall), &(cmp.Overall), newVal.Overall, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                return false;
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

                DO_NOTHING();
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
                DO_NOTHING();

                copy = {this->Value.Overall};
            }
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_CHK;
        }

        /**
         *  Acquire the ticket lock, waiting if necessary.
         */
        __forceinline void Acquire() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            uint16_t const myTicket = __atomic_fetch_add(&(this->Value.Head), 1, __ATOMIC_ACQUIRE);

            uint16_t diff;

            while ((diff = myTicket - this->Value.Tail) != 0)
                do DO_NOTHING(); while (--diff != 0);
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;
        }

        /**
         *  Release the ticket lock.
         */
        __forceinline void Release() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            ++this->Value.Tail;
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
         *  Acquire the ticket lock, waiting if necessary.
         *  Includes a pointer in the memory barrier, if supported.
         */
        __forceinline void SimplyAcquire() volatile { this->Acquire(); }

        /**
         *  Release the ticket lock.
         */
        __forceinline void SimplyRelease() volatile { this->Release(); }

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
     *  Busy-waiting re-entrant synchronization primitive.
     */
    template<>
    struct TicketLock<true>
    {
    public:

        typedef void Cookie;

        /*  Constructor(s)  */

        TicketLock() = default;
        TicketLock(TicketLock const &) = delete;
        TicketLock & operator =(TicketLock const &) = delete;
        TicketLock(TicketLock &&) = delete;
        TicketLock & operator =(TicketLock &&) = delete;

        /*  Operations  */

        __forceinline __must_check constexpr bool TryAcquire() const volatile
        { return true; }

        __forceinline void Spin() const volatile { }
        __forceinline void Await() const volatile { }

        __forceinline void Acquire() const volatile { }
        __forceinline void SimplyAcquire() const volatile { }

        __forceinline void Release() const volatile { }
        __forceinline void SimplyRelease() const volatile { }

        __forceinline __must_check constexpr bool Check() const volatile
        { return true; }

        /*  Properties  */

        __forceinline constexpr ticketlock_t GetValue() const volatile
        { return (ticketlock_t)0; }

        /*  Fields  */

    private:

        ticketlock_t Value; 
    };
#endif
}}
