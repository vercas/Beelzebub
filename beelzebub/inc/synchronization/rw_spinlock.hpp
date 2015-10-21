#pragma once

#include <synchronization/atomic.hpp>
#include <system/cpu.hpp>

namespace Beelzebub { namespace Synchronization
{
    //  The first version (on non-SMP builds) is dumbed down.

    struct RwSpinlock
    {
    public:

        /*  Constructor(s)  */

        RwSpinlock() = default;
        RwSpinlock(RwSpinlock const &) = delete;
        RwSpinlock & operator =(RwSpinlock const &) = delete;

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)

        /*  Acquisition Operations  */

        /**
         *  <summary>Acquires the lock as a reader.</summary>
         */
        __bland __forceinline void AcquireAsReader() volatile
        {
            this->ReaderCount = 1;
        }

        /**
         *  <summary>Acquires the lock as the writer.</summary>
         */
        __bland __forceinline void AcquireAsWriter() volatile
        {
            //  Nuthin'.
        }

        /**
         *  <summary>Upgrades a reader to the writer.</summary>
         */
        __bland __forceinline void UpgradeToWriter() volatile
        {
            this->ReaderCount = 0;
        }

        /*  Release Operations  */

        /**
         *  <summary>Releases the lock as a reader.</summary>
         */
        __bland __forceinline void ReleaseAsReader() volatile
        {
            this->ReaderCount = 0;
        }

        /**
         *  <summary>Releases the lock as the writer.</summary>
         */
        __bland __forceinline void ReleaseAsWriter() volatile
        {
            //  Nuthin'.
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
        __bland inline void AcquireAsReader() volatile
        {
            while (this->Lock.TestSet())
                Cpu::DoNothing();
            //  Lock.

            ++this->ReaderCount;
            //  Add reader.

            this->Lock.Clear();
            //  Unlock.
        }

        /**
         *  <summary>Acquires the lock as the writer.</summary>
         */
        __bland inline void AcquireAsWriter() volatile
        {
            while (this->Lock.TestSet())
                Cpu::DoNothing();
            //  Lock.

            while (this->ReaderCount.Load() > 0)
                Cpu::DoNothing();
            //  Wait for all readers to finish.
        }

        /**
         *  <summary>Upgrades a reader to the writer.</summary>
         */
        __bland inline void UpgradeToWriter() volatile
        {
            this->Lock.TestSet();
            //  Cannot fail.

            --this->ReaderCount;
            //  Remove reader.

            while (this->ReaderCount.Load() > 0)
                Cpu::DoNothing();
            //  Wait for the rest of the readers to finish.
        }

        /*  Release Operations  */

        /**
         *  <summary>Releases the lock as a reader.</summary>
         */
        __bland inline void ReleaseAsReader() volatile
        {
            --this->ReaderCount;
            //  Remove reader.

            //  Nothing else really needs to be done.
        }

        /**
         *  <summary>Releases the lock as the writer.</summary>
         */
        __bland inline void ReleaseAsWriter() volatile
        {
            this->Lock.Clear();
            //  Unlock.

            //  Nothing else really needs to be done here either.
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
