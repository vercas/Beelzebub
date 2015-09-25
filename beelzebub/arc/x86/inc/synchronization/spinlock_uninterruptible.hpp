/**
 *  The `__must_check` attributes are there to make sure that the SpinlockUninterruptible
 *  is not used in place of a normal spinlock accidentally.
 */

#pragma once

#include <metaprogramming.h>
#include <system/cpu.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

namespace Beelzebub { namespace Synchronization
{
    typedef vsize_t spinlock_t;

    /**
     *  Busy-waiting re-entrant synchronization primitive which
     *  prevents CPU interrupts on the locking CPU.
     */
    struct SpinlockUninterruptible
    {
    public:

        /*  Constructor(s)  */

        SpinlockUninterruptible() = default;
        SpinlockUninterruptible(SpinlockUninterruptible const &) = delete;
        SpinlockUninterruptible & operator =(const SpinlockUninterruptible &) = delete;

        /*  Destructor  */

#ifdef __BEELZEBUB__DEBUG
        __bland ~SpinlockUninterruptible();
#endif

        /*  Operations  */

        /**
         *  Acquire the spinlock, if possible.
         */
        __bland __forceinline __must_check bool TryAcquire(int_cookie_t & int_cookie)
        {
            int_cookie = Cpu::PushDisableInterrupts();

            spinlock_t oldValue = __sync_lock_test_and_set(&this->Value, 1);

            if (oldValue != 0)
                Cpu::RestoreInterruptState(int_cookie);
            //  If the spinlock was already locked, restore interrupt state.

            return !oldValue;
        }

        /**
         *  Awaits for the spinlock to be freed.
         *  Does not acquire the lock.
         */
        __bland __forceinline void Spin()
        {
            do
            {
                asm volatile ("pause");
            } while (this->Value);
        }

        /**
         *  Checks if the spinlock is free. If not, it awaits.
         *  Does not acquire the lock.
         */
        __bland __forceinline void Await()
        {
            while (this->Value)
            {
                asm volatile ("pause");
            }
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __bland __forceinline __must_check int_cookie_t Acquire()
        {
            const int_cookie_t int_cookie = Cpu::PushDisableInterrupts();

            while (__sync_lock_test_and_set(&this->Value, 1))
                this->Spin();

            return int_cookie;
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __bland __forceinline void SimplyAcquire()
        {
            while (__sync_lock_test_and_set(&this->Value, 1))
                this->Spin();
        }

        /**
         *  Release the spinlock.
         */
        __bland __forceinline void Release(const int_cookie_t int_cookie)
        {
            __sync_lock_release(&this->Value);

            Cpu::RestoreInterruptState(int_cookie);
        }

        /**
         *  Release the spinlock.
         */
        __bland __forceinline void SimplyRelease()
        {
            __sync_lock_release(&this->Value);
        }

        /**
         *  Checks whether the spinlock is free or not.
         */
        __bland __forceinline __must_check bool Check()
        {
            return this->Value == 0;
        }

        /*  Properties  */

        __bland __forceinline spinlock_t GetValue()
        {
            return this->Value;
        }

        /*  Fields  */

    private:

        volatile spinlock_t Value; 
    };
}}

//  Very sad note: GCC doesn't support protecting additional pointers in
//  the memory barrier. Nevertheless, I have added the feature.
