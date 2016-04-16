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
 *  The `__must_check` attributes are there to make sure that the SpinlockUninterruptible
 *  is not used in place of a normal spinlock accidentally.
 */

#pragma once

#include <synchronization/lock_guard.hpp>
#include <system/interrupts.hpp>

namespace Beelzebub { namespace Synchronization
{
#ifndef __BEELZEBUB_SPINLOCK_T
#define __BEELZEBUB_SPINLOCK_T
    typedef union spinlock_t
    {
        uint32_t volatile Overall;
        struct
        {
            uint16_t Head;
            uint16_t Tail;
        };

        spinlock_t() = default;
        inline spinlock_t(uint32_t o) : Overall(o) { }
        inline spinlock_t(uint16_t h, uint16_t t) : Head(h), Tail(t) { }
    } spinlock_t;
#endif

    static_assert(sizeof(spinlock_t) == 4, "");

    //  Lemme clarify here.
    //  For non-SMP builds, SMP spinlocks are gonna be dummies.
    //  For SMP builds, all spinlocks are implemented.

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    /**
     *  Busy-waiting re-entrant synchronization primitive which
     *  prevents CPU interrupts on the locking CPU.
     */
    template<bool SMP = true>
    struct SpinlockUninterruptible { };

    /**
     *  Busy-waiting re-entrant synchronization primitive which
     *  prevents CPU interrupts on the locking CPU.
     */
    template<>
    struct SpinlockUninterruptible<false>
#else
    /**
     *  Busy-waiting re-entrant synchronization primitive which
     *  prevents CPU interrupts on the locking CPU.
     */
    template<bool SMP = true>
    struct SpinlockUninterruptible
#endif
    {
    public:

        typedef int_cookie_t Cookie;

        /*  Constructor(s)  */

        SpinlockUninterruptible() = default;
        SpinlockUninterruptible(SpinlockUninterruptible const &) = delete;
        SpinlockUninterruptible & operator =(SpinlockUninterruptible const &) = delete;

        /*  Destructor  */

#ifdef __BEELZEBUB__DEBUG
        ~SpinlockUninterruptible();
#endif

        /*  Operations  */

#ifdef __BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS
        /**
         *  Acquire the spinlock, if possible.
         */
        __noinline __must_check bool TryAcquire(Cookie & cookie) volatile;

        /**
         *  Awaits for the spinlock to be freed.
         *  Does not acquire the lock.
         */
        __noinline void Spin() const volatile;

        /**
         *  Checks if the spinlock is free. If not, it awaits.
         *  Does not acquire the lock.
         */
        __noinline void Await() const volatile;

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __noinline __must_check Cookie Acquire() volatile;

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __noinline void SimplyAcquire() volatile;

        /**
         *  Release the spinlock.
         */
        __noinline void Release(Cookie const cookie) volatile;

        /**
         *  Release the spinlock.
         */
        __noinline void SimplyRelease() volatile;

        /**
         *  Checks whether the spinlock is free or not.
         */
        __noinline __must_check bool Check() const volatile;
#else
        /**
         *  Acquire the spinlock, if possible.
         */
        __forceinline __must_check bool TryAcquire(Cookie & cookie) volatile
        {
            cookie = System::Interrupts::PushDisable();

            COMPILER_MEMORY_BARRIER();

        op_start:
            uint16_t const oldTail = this->Value.Tail;
            spinlock_t cmp {oldTail, oldTail};
            spinlock_t const newVal {oldTail, (uint16_t)(oldTail + 1)};
            spinlock_t const cmpCpy = cmp;

            asm volatile( "lock cmpxchgl %[newVal], %[curVal] \n\t"
                        : [curVal]"+m"(this->Value), "+a"(cmp)
                        : [newVal]"r"(newVal)
                        : "flags" );

            if likely(cmp.Overall == cmpCpy.Overall)
            {
                System::Interrupts::RestoreState(cookie);
                //  If the spinlock was already locked, restore interrupt state.

                return false;
            }
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;

            return true;
        }

        /**
         *  Awaits for the spinlock to be freed.
         *  Does not acquire the lock.
         */
        __forceinline void Spin() const volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            spinlock_t copy;

            do
            {
                copy = {this->Value.Overall};

                asm volatile ( "pause \n\t" : : : "memory" );
            } while (copy.Tail != copy.Head);
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_CHK;
        }

        /**
         *  Checks if the spinlock is free. If not, it awaits.
         *  Does not acquire the lock.
         */
        __forceinline void Await() const volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            spinlock_t copy = {this->Value.Overall};

            while (copy.Tail != copy.Head)
            {
                asm volatile ( "pause \n\t" : : : "memory" );

                copy = {this->Value.Overall};
            }
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_CHK;
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __forceinline __must_check Cookie Acquire() volatile
        {
            Cookie const cookie = System::Interrupts::PushDisable();

            COMPILER_MEMORY_BARRIER();

        op_start:
            uint16_t myTicket = 1;

            asm volatile( "lock xaddw %[ticket], %[tail] \n\t"
                        : [tail]"+m"(this->Value.Tail)
                        , [ticket]"+r"(myTicket)
                        : : "flags" );
            //  It's possible to address the upper word directly.

            while (this->Value.Head != myTicket)
                asm volatile ( "pause \n\t" : : : "memory" );
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;

            return cookie;
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __forceinline void SimplyAcquire() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            uint16_t myTicket = 1;

            asm volatile( "lock xaddw %[ticket], %[tail] \n\t"
                        : [tail]"+m"(this->Value.Tail)
                        , [ticket]"+r"(myTicket)
                        : : "flags" );
            //  It's possible to address the upper word directly.

            while (this->Value.Head != myTicket)
                asm volatile ( "pause \n\t" : : : "memory" );
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;
        }

        /**
         *  Release the spinlock.
         */
        __forceinline void Release(Cookie const cookie) volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            asm volatile( "lock addw $1, %[head] \n\t"
                        : [head]"+m"(this->Value.Head)
                        : : "flags" );
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_REL;

            System::Interrupts::RestoreState(cookie);
        }

        /**
         *  Release the spinlock.
         */
        __forceinline void SimplyRelease() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            asm volatile( "lock addw $1, %[head] \n\t"
                        : [head]"+m"(this->Value.Head)
                        : : "flags" );
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_REL;
        }

        /**
         *  Checks whether the spinlock is free or not.
         */
        __forceinline __must_check bool Check() const volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            spinlock_t copy = {this->Value.Overall};

            if (copy.Head != copy.Tail)
                return false;
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_CHK;

            return true;
        }
#endif

        /**
         *  Reset the spinlock.
         */
        __forceinline void Reset() volatile
        {
            this->Value.Overall = 0;
        }

        /*  Fields  */

    private:

        spinlock_t Value;
    };

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    /**
     *  Busy-waiting re-entrant synchronization primitive which
     *  prevents CPU interrupts on the locking CPU.
     */
    template<>
    struct SpinlockUninterruptible<true>
    {
    public:

        typedef int_cookie_t Cookie;

        /*  Constructor(s)  */

        SpinlockUninterruptible() = default;
        SpinlockUninterruptible(SpinlockUninterruptible const &) = delete;
        SpinlockUninterruptible & operator =(SpinlockUninterruptible const &) = delete;

        /*  Operations  */

        __forceinline __must_check bool TryAcquire(Cookie & cookie) const volatile
        { cookie = System::Interrupts::PushDisable(); return true; }
        __forceinline void Spin() const volatile { }
        __forceinline void Await() const volatile { }
        __forceinline __must_check Cookie Acquire() const volatile
        { return System::Interrupts::PushDisable(); }
        __forceinline void SimplyAcquire() const volatile { }

        __forceinline void Release(Cookie const cookie) const volatile
        { System::Interrupts::RestoreState(cookie); }
        __forceinline void SimplyRelease() const volatile { }
        __forceinline __must_check bool Check() const volatile
        { return true; }
    };
#endif
}}

//  Very sad note: GCC doesn't support protecting additional pointers in
//  the memory barrier. Nevertheless, I have added the feature.
