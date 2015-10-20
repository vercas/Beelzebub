#pragma once

#include <synchronization/spinlock_uninterruptible.hpp>
#include <synchronization/atomic.hpp>
#include <system/cpu.hpp>

namespace Beelzebub { namespace Synchronization
{
	//	The first version (on non-SMP builds) is dumbed down.

	struct RwLock
	{
    public:

        /*  Constructor(s)  */

        RwLock() = default;
        RwLock(RwLock const &) = delete;
        RwLock & operator =(RwLock const &) = delete;

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)

        /*  Operations  */

        /**
         *  Resets the barrier, so it will open when the specified number
         *  of cores reach it.
         */
        __bland __forceinline void Reset(size_t const coreCount = 1) volatile
        {
            this->Value = 1;
        }

        /**
         *  Reach the barrier, awaiting for other cores if necessary.
         */
        __bland __forceinline void Reach() volatile
        {
            this.Value = 0;
        }

        /*  Properties  */

        /**
         *  Checks whether the spinlock is free or not.
         */
        __bland __forceinline __must_check bool IsOpen() const volatile
        {
            return this->Value == (size_t)0;
        }

        __bland __forceinline size_t GetValue() const volatile
        {
            return this->Value;
        }

        /*  Fields  */

    private:

        size_t volatile Value;

#else

        /*  Operations  */

        /**
         *  Resets the barrier, so it will open when all cores reach it.
         */
        __bland __forceinline void Reset() volatile
        {
            this->Value.Store(System::Cpu::Count);
        }

        /**
         *  Resets the barrier, so it will open when the specified number
         *  of cores reach it.
         */
        __bland __forceinline void Reset(size_t const coreCount) volatile
        {
            this->Value.Store(coreCount);
        }

        /**
         *  Reach the barrier, awaiting for other cores if necessary.
         */
        __bland inline void Reach() volatile
        {
            --this->Value;

            while (this->Value)
            	System::Cpu::DoNothing();
        }

        /*  Properties  */

        /**
         *  Checks whether the spinlock is free or not.
         */
        __bland __forceinline __must_check bool IsOpen() const volatile
        {
            return this->Value == (size_t)0;
        }

        __bland __forceinline size_t GetValue() const volatile
        {
            return this->Value.Load();
        }

        /*  Fields  */

    private:

        Atomic<size_t> volatile Readers; 
        SpinlockUninterruptible<> Writer;
#endif
	};
}}
