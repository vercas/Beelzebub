#pragma once

#include <synchronization/atomic.hpp>
#include <system/interrupts.hpp>
#include <system/cpu_instructions.hpp>

namespace Beelzebub { namespace Synchronization
{
    //  The first version (on non-SMP builds) is dumbed down.

    struct RwSpinlockUninterruptible
    {
    public:

        typedef int_cookie_t Cookie;

        /*  Constructor(s)  */

        __bland inline RwSpinlockUninterruptible()
            : ReaderCount( 0)
            , Lock(false)
        {
            //  Just make sure it's not in a bad state.
        }

        RwSpinlockUninterruptible(RwSpinlockUninterruptible const &) = delete;
        RwSpinlockUninterruptible & operator =(RwSpinlockUninterruptible const &) = delete;

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)

        /*  Acquisition Operations  */

        /**
         *  <summary>Acquires the lock as a reader.</summary>
         */
        __bland __forceinline Cookie AcquireAsReader() volatile
        {
            this->ReaderCount = 1;
            
            return System::Interrupts::PushDisable();
        }

        /**
         *  <summary>Acquires the lock as a reader.</summary>
         */
        __bland __forceinline void SimplyAcquireAsReader() volatile
        {
            this->ReaderCount = 1;
        }

        /**
         *  <summary>Acquires the lock as the writer.</summary>
         */
        __bland __forceinline Cookie AcquireAsWriter() volatile
        {
            return System::Interrupts::PushDisable();
        }

        /**
         *  <summary>Acquires the lock as the writer.</summary>
         */
        __bland __forceinline void SimplyAcquireAsWriter() volatile
        {
            //  Nothing.
        }

        /**
         *  <summary>Upgrades a reader to the writer.</summary>
         */
        __bland __forceinline bool UpgradeToWriter() volatile
        {
            this->ReaderCount = 0;

            return true;
        }

        /*  Release Operations  */

        /**
         *  <summary>Releases the lock as a reader.</summary>
         */
        __bland __forceinline void ReleaseAsReader(Cookie const cookie) volatile
        {
            this->ReaderCount = 0;

            System::Interrupts::RestoreState(cookie);
        }

        /**
         *  <summary>Releases the lock as a reader.</summary>
         */
        __bland __forceinline void SimplyReleaseAsReader() volatile
        {
            this->ReaderCount = 0;
        }

        /**
         *  <summary>Releases the lock as the writer.</summary>
         */
        __bland __forceinline void ReleaseAsWriter(Cookie const cookie) volatile
        {
            System::Interrupts::RestoreState(cookie);
        }

        /**
         *  <summary>Releases the lock as the writer.</summary>
         */
        __bland __forceinline void SimplyReleaseAsWriter() volatile
        {
            //  Nothing.
        }

        /**
         *  <summary>Downgrades the writer to a reader.</summary>
         */
        __bland __forceinline void DowngradeToReader() volatile
        {
            this->ReaderCount = 1;
        }

        /*  Properties  */

        /**
         *  Gets the number of active readers.</summary>
         *  <return>The number of active readers.</return>
         */
        __bland __forceinline __must_check size_t GetReaderCount() const volatile
        {
            return (size_t)this->ReaderCount;
        }

        /*  Fields  */

    private:

        char volatile ReaderCount;

#else

        /*  Acquisition Operations  */

        /**
         *  <summary>Acquires the lock as a reader.</summary>
         */
        __bland inline Cookie AcquireAsReader() volatile
        {
            Cookie const cookie = System::Interrupts::PushDisable();
            
            this->SimplyAcquireAsReader();

            return cookie;
        }

        /**
         *  <summary>Acquires the lock as a reader.</summary>
         */
        __bland inline void SimplyAcquireAsReader() volatile
        {
            while (this->Lock.TestSet())
                System::CpuInstructions::DoNothing();
            //  Lock.

            ++this->ReaderCount;
            //  Add reader.

            this->Lock.Clear();
            //  Unlock.
        }

        /**
         *  <summary>Acquires the lock as the writer.</summary>
         */
        __bland inline Cookie AcquireAsWriter() volatile
        {
            Cookie const cookie = System::Interrupts::PushDisable();
            
            this->SimplyAcquireAsWriter();

            return cookie;
        }

        /**
         *  <summary>Acquires the lock as the writer.</summary>
         */
        __bland inline void SimplyAcquireAsWriter() volatile
        {
            while (this->Lock.TestSet())
                System::CpuInstructions::DoNothing();
            //  Lock.

            while (this->ReaderCount.Load() > 0)
                System::CpuInstructions::DoNothing();
            //  Wait for all readers to finish.
        }

        /**
         *  <summary>Upgrades a reader to the writer.</summary>
         */
        __bland inline bool UpgradeToWriter() volatile
        {
            if (this->Lock.TestSet())
            {
                //  There's nothing to do.

                return false;
            }
            else
            {
                //  Just acquired the writer lock.

                --this->ReaderCount;
                //  Remove reader.

                while (this->ReaderCount.Load() > 0)
                    System::CpuInstructions::DoNothing();
                //  Wait for the rest of the readers to finish.

                return true;
            }
        }

        /*  Release Operations  */

        /**
         *  <summary>Releases the lock as a reader.</summary>
         */
        __bland inline void ReleaseAsReader(Cookie const cookie) volatile
        {
            this->SimplyReleaseAsReader();

            System::Interrupts::RestoreState(cookie);
        }

        /**
         *  <summary>Releases the lock as a reader.</summary>
         */
        __bland inline void SimplyReleaseAsReader() volatile
        {
            --this->ReaderCount;
            //  Remove reader.
        }

        /**
         *  <summary>Releases the lock as the writer.</summary>
         */
        __bland inline void ReleaseAsWriter(Cookie const cookie) volatile
        {
            this->SimplyReleaseAsWriter();

            System::Interrupts::RestoreState(cookie);
        }

        /**
         *  <summary>Releases the lock as the writer.</summary>
         */
        __bland inline void SimplyReleaseAsWriter() volatile
        {
            this->Lock.Clear();
            //  Unlock.
        }

        /**
         *  <summary>Downgrades the writer to a reader.</summary>
         */
        __bland inline void DowngradeToReader() volatile
        {
            ++this->ReaderCount;
            //  Add reader.

            this->Lock.Clear();
            //  Unlock.

            //  That's it, folks.
        }

        /*  Properties  */

        /**
         *  Gets the number of active readers.</summary>
         *  <return>The number of active readers.</return>
         */
        __bland __forceinline __must_check size_t GetReaderCount() const volatile
        {
            return this->ReaderCount.Load();
        }

        /*  Fields  */

    private:

        Atomic<size_t> volatile ReaderCount;
        Atomic<bool> Lock;
#endif
    };
}}
