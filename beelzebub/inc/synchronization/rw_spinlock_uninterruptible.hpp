#pragma once

#include <synchronization/atomic.hpp>
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
#if   !defined(__BEELZEBUB_SETTINGS_NO_SMP)
            , Lock(false)
#endif
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
