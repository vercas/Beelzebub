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
#include <system/cpu_instructions.hpp>

namespace Beelzebub { namespace Synchronization
{
    typedef size_t volatile spinlock_t;

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

        /**
         *  Acquire the spinlock, if possible.
         */
        __forceinline __must_check bool TryAcquire(Cookie & cookie) volatile
        {
            cookie = System::Interrupts::PushDisable();

            spinlock_t oldValue = __sync_lock_test_and_set(&this->Value, 1);

            if (oldValue != 0)
                System::Interrupts::RestoreState(cookie);
            //  If the spinlock was already locked, restore interrupt state.

            return !oldValue;
        }

        /**
         *  Awaits for the spinlock to be freed.
         *  Does not acquire the lock.
         */
        __forceinline void Spin() const volatile
        {
            do
            {
                System::CpuInstructions::DoNothing();
            } while (this->Value);
        }

        /**
         *  Checks if the spinlock is free. If not, it awaits.
         *  Does not acquire the lock.
         */
        __forceinline void Await() const volatile
        {
            while (this->Value)
            {
                System::CpuInstructions::DoNothing();
            }
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __forceinline __must_check Cookie Acquire() volatile
        {
            Cookie const cookie = System::Interrupts::PushDisable();

            while (__sync_lock_test_and_set(&this->Value, 1))
                this->Spin();

            return cookie;
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __forceinline void SimplyAcquire() volatile
        {
            while (__sync_lock_test_and_set(&this->Value, 1))
                this->Spin();
        }

        /**
         *  Release the spinlock.
         */
        __forceinline void Release(Cookie const cookie) volatile
        {
            __sync_lock_release(&this->Value);

            System::Interrupts::RestoreState(cookie);
        }

        /**
         *  Release the spinlock.
         */
        __forceinline void SimplyRelease() volatile
        {
            __sync_lock_release(&this->Value);
        }

        /**
         *  Checks whether the spinlock is free or not.
         */
        __forceinline __must_check bool Check() const volatile
        {
            return this->Value == 0;
        }

        /*  Properties  */

        __forceinline spinlock_t GetValue() const volatile
        {
            return this->Value;
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

        /**
         *  Acquire the spinlock, if possible.
         */
        __forceinline __must_check bool TryAcquire(Cookie & cookie) const volatile
        {
            cookie = System::Interrupts::PushDisable();

            return true;
        }

        /**
         *  Awaits for the spinlock to be freed.
         *  Does not acquire the lock.
         */
        __forceinline void Spin() const volatile
        {
            //  Do nothing.
        }

        /**
         *  Checks if the spinlock is free. If not, it awaits.
         *  Does not acquire the lock.
         */
        __forceinline void Await() const volatile
        {
            //  Do nothing.
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __forceinline __must_check Cookie Acquire() const volatile
        {
            return System::Interrupts::PushDisable();
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __forceinline void SimplyAcquire() const volatile
        {
            //  Do nothing.
        }

        /**
         *  Release the spinlock.
         */
        __forceinline void Release(Cookie const cookie) const volatile
        {
            System::Interrupts::RestoreState(cookie);
        }

        /**
         *  Release the spinlock.
         */
        __forceinline void SimplyRelease() const volatile
        {
            //  Do nothing.
        }

        /**
         *  Checks whether the spinlock is free or not.
         */
        __forceinline __must_check bool Check() const volatile
        {
            return true;
        }

        /*  Properties  */

        __forceinline spinlock_t GetValue() const volatile
        {
            return (spinlock_t)0;
        }

        /*  Fields  */

    private:

        volatile spinlock_t Value; 
    };
#endif
}}

//  Very sad note: GCC doesn't support protecting additional pointers in
//  the memory barrier. Nevertheless, I have added the feature.
