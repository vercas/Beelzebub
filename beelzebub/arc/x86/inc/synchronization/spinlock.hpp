#pragma once

#include <debug.hpp>
#include <metaprogramming.h>

namespace Beelzebub { namespace Synchronization
{
    typedef vsize_t spinlock_t;

    /*
     *  Busy-waiting re-entrant synchronization primitive.
     */
    struct Spinlock
    {
    public:

        /*  Constructor(s)  */

        Spinlock() = default;
        Spinlock(Spinlock const&) = delete;

        /*  Destructor  */

#ifdef __BEELZEBUB__DEBUG
        ~Spinlock();
#endif

        /*  Operations  */

        /*
         *  Acquire the spinlock, if possible.
         */
        __bland __forceinline bool TryAcquire()
        {
            spinlock_t oldValue = __sync_lock_test_and_set(&this->Value, 1);

            //msg("LOCK|TAQ|%Xp|%s%n", this, oldValue ? "BUSY" : "ACQUIRED");

            return !oldValue;
        }

        /*
         *  Awaits for the spinlock to be freed.
         *  Does not acquire the lock.
         */
        __bland __forceinline void Spin()
        {
            do
            {
                asm volatile ("pause");

                //msg("LOCK|SPN|%Xp|%s|%Xs%n", this, this->Value ? "BUSY" : "FREE", this->Value);
            } while (this->Value);
        }

        /*
         *  Acquire the spinlock, waiting if necessary.
         */
        __bland __forceinline void Acquire()
        {
            while (__sync_lock_test_and_set(&this->Value, 1))
                this->Spin();

            //msg("LOCK|ACQ|%Xp%n", this);
        }

        /*
         *  Acquire the spinlock, waiting if necessary.
         *  Includes a pointer in the memory barrier, if supported.
         */
        __bland __forceinline void Acquire(void * const ptr)
        {
            while (__sync_lock_test_and_set(&this->Value, 1, ptr))
                this->Spin();

            //msg("LOCK|ACQ|%Xp%n", this);
        }

        /*
         *  Release the spinlock.
         */
        __bland __forceinline void Release()
        {
            //msg("LOCK|REL|%Xp%n", this);

            __sync_lock_release(&this->Value);
        }

        /*
         *  Release the spinlock.
         *  Includes a pointer in the memory barrier.
         */
        __bland __forceinline void Release(void * const ptr)
        {
            //msg("LOCK|REL|%Xp%n", this);

            __sync_lock_release(&this->Value, ptr);
        }

        /*
         *  Checks whether the spinlock is free or not.
         */
        __bland __forceinline bool Check()
        {
            //msg("LOCK|CHK|%Xp|%s%n", this, this->Value ? "BUSY" : "FREE");

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

//  Very sad note: GCC doesn't support protectign additional pointers in
//  the memory barrier. Nevertheless, I have added the feature.
