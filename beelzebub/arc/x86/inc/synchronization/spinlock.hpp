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

#include <system/cpu_instructions.hpp>

namespace Beelzebub { namespace Synchronization
{
    typedef size_t volatile spinlock_t;

    //  Lemme clarify here.
    //  For non-SMP builds, SMP spinlocks are gonna be dummies.
    //  For SMP builds, all spinlocks are implemented.

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    /**
     *  Busy-waiting re-entrant synchronization primitive.
     */
    template<bool SMP = true>
    struct Spinlock { };

    /**
     *  Busy-waiting re-entrant synchronization primitive.
     */
    template<>
    struct Spinlock<false>
#else
    /**
     *  Busy-waiting re-entrant synchronization primitive.
     */
    template<bool SMP = true>
    struct Spinlock
#endif
    {
    public:

        typedef void Cookie;

        /*  Constructor(s)  */

        Spinlock() = default;
        Spinlock(Spinlock const &) = delete;
        Spinlock & operator =(Spinlock const &) = delete;

        /*  Destructor  */

#ifdef __BEELZEBUB__DEBUG
        ~Spinlock();
#endif

        /*  Operations  */

        /**
         *  Acquire the spinlock, if possible.
         */
        __forceinline __must_check bool TryAcquire() volatile
        {
            spinlock_t oldValue = __sync_lock_test_and_set(&this->Value, 1);

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
                System::CpuInstructions::DoNothing();
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __forceinline void Acquire() volatile
        {
            while (__sync_lock_test_and_set(&this->Value, 1))
                this->Spin();
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         *  Includes a pointer in the memory barrier, if supported.
         */
        __forceinline void SimplyAcquire() volatile { this->Acquire(); }

        /**
         *  Release the spinlock.
         */
        __forceinline void Release() volatile
        {
            __sync_lock_release(&this->Value);
        }

        /**
         *  Release the spinlock.
         */
        __forceinline void SimplyRelease() volatile { this->Release(); }

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
     *  Busy-waiting re-entrant synchronization primitive.
     */
    template<>
    struct Spinlock<true>
    {
    public:

        typedef void Cookie;

        /*  Constructor(s)  */

        Spinlock() = default;
        Spinlock(Spinlock const &) = delete;
        Spinlock & operator =(Spinlock const &) = delete;

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

        __forceinline constexpr spinlock_t GetValue() const volatile
        { return (spinlock_t)0; }
    };
#endif
}}

//  Very sad note: GCC doesn't support protecting additional pointers in
//  the memory barrier. Nevertheless, I have added the feature.
