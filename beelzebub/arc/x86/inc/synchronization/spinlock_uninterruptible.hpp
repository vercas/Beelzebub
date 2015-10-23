/**
 *  The `__must_check` attributes are there to make sure that the SpinlockUninterruptible
 *  is not used in place of a normal spinlock accidentally.
 */

#pragma once

#include <system/interrupts.hpp>
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

        /*  Constructor(s)  */

        SpinlockUninterruptible() = default;
        SpinlockUninterruptible(SpinlockUninterruptible const &) = delete;
        SpinlockUninterruptible & operator =(SpinlockUninterruptible const &) = delete;

        /*  Destructor  */

#ifdef __BEELZEBUB__DEBUG
        __bland ~SpinlockUninterruptible();
#endif

        /*  Operations  */

        /**
         *  Acquire the spinlock, if possible.
         */
        __bland __forceinline __must_check bool TryAcquire(int_cookie_t & int_cookie) volatile
        {
            int_cookie = System::Interrupts::PushDisable();

            spinlock_t oldValue = __sync_lock_test_and_set(&this->Value, 1);

            if (oldValue != 0)
                System::Interrupts::RestoreState(int_cookie);
            //  If the spinlock was already locked, restore interrupt state.

            return !oldValue;
        }

        /**
         *  Awaits for the spinlock to be freed.
         *  Does not acquire the lock.
         */
        __bland __forceinline void Spin() const volatile
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
        __bland __forceinline void Await() const volatile
        {
            while (this->Value)
            {
                System::CpuInstructions::DoNothing();
            }
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __bland __forceinline __must_check int_cookie_t Acquire() volatile
        {
            const int_cookie_t int_cookie = System::Interrupts::PushDisable();

            while (__sync_lock_test_and_set(&this->Value, 1))
                this->Spin();

            return int_cookie;
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __bland __forceinline void SimplyAcquire() volatile
        {
            while (__sync_lock_test_and_set(&this->Value, 1))
                this->Spin();
        }

        /**
         *  Release the spinlock.
         */
        __bland __forceinline void Release(int_cookie_t const int_cookie) volatile
        {
            __sync_lock_release(&this->Value);

            System::Interrupts::RestoreState(int_cookie);
        }

        /**
         *  Release the spinlock.
         */
        __bland __forceinline void SimplyRelease() volatile
        {
            __sync_lock_release(&this->Value);
        }

        /**
         *  Checks whether the spinlock is free or not.
         */
        __bland __forceinline __must_check bool Check() const volatile
        {
            return this->Value == 0;
        }

        /*  Properties  */

        __bland __forceinline spinlock_t GetValue() const volatile
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

        /*  Constructor(s)  */

        SpinlockUninterruptible() = default;
        SpinlockUninterruptible(SpinlockUninterruptible const &) = delete;
        SpinlockUninterruptible & operator =(SpinlockUninterruptible const &) = delete;

        /*  Operations  */

        /**
         *  Acquire the spinlock, if possible.
         */
        __bland __forceinline __must_check bool TryAcquire(int_cookie_t & int_cookie) const volatile
        {
            int_cookie = System::Interrupts::PushDisable();

            return true;
        }

        /**
         *  Awaits for the spinlock to be freed.
         *  Does not acquire the lock.
         */
        __bland __forceinline void Spin() const volatile
        {
            //  Do nothing.
        }

        /**
         *  Checks if the spinlock is free. If not, it awaits.
         *  Does not acquire the lock.
         */
        __bland __forceinline void Await() const volatile
        {
            //  Do nothing.
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __bland __forceinline __must_check int_cookie_t Acquire() const volatile
        {
            return System::Interrupts::PushDisable();
        }

        /**
         *  Acquire the spinlock, waiting if necessary.
         */
        __bland __forceinline void SimplyAcquire() const volatile
        {
            //  Do nothing.
        }

        /**
         *  Release the spinlock.
         */
        __bland __forceinline void Release(int_cookie_t const int_cookie) const volatile
        {
            System::Interrupts::RestoreState(int_cookie);
        }

        /**
         *  Release the spinlock.
         */
        __bland __forceinline void SimplyRelease() const volatile
        {
            //  Do nothing.
        }

        /**
         *  Checks whether the spinlock is free or not.
         */
        __bland __forceinline __must_check bool Check() const volatile
        {
            return true;
        }

        /*  Properties  */

        __bland __forceinline spinlock_t GetValue() const volatile
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
