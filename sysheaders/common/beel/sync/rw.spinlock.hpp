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

#include <beel/sync/atomic.hpp>

namespace Beelzebub { namespace Synchronization
{
    //  The second version (on non-SMP builds) is dumbed down.

    struct RwSpinlock
    {
        /*  Constants  */

        static constexpr uint32_t const Wait = 1;
        static constexpr uint32_t const Write = 2;
        static constexpr uint32_t const Read = 4;

        static constexpr uint32_t const ReadMask = ~(Wait | Write);

        static constexpr uint32_t const WaitBit = 0;
        static constexpr uint32_t const WriteBit = 1;
        static constexpr uint32_t const ReadBit = 2;

        /*  Constructor(s)  */

        RwSpinlock() = default;

        RwSpinlock(RwSpinlock const &) = delete;
        RwSpinlock & operator =(RwSpinlock const &) = delete;
        RwSpinlock(RwSpinlock &&) = delete;
        RwSpinlock & operator =(RwSpinlock &&) = delete;

#ifdef __BEELZEBUB_SETTINGS_SMP

        /*  Acquisition Operations  */

        /**
         *  <summary>Attempts to acquire the lock as a reader.</summary>
         *  <return>True if the acquisition succeeded; false otherwise.</return>
         */
        inline __must_check bool TryAcquireAsReader() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            uint32_t val = this->Value.FetchAdd(Read);
            //  Attempt registration as reader.

            if (0 != (val & (Wait | Write)))
            {
                //  It appears a writer is awaiting.

                this->Value.FetchSub(Read);
                //  Take a step back.

                return false;
                //  Fail.
            }
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;

            return true;
        }

        /**
         *  <summary>Acquires the lock as a reader.</summary>
         */
        inline void AcquireAsReader() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            while (true)
            {
                while (this->Value.Load() & (Wait | Write))
                    asm volatile ( "pause \n\t" : : : "memory" );
                //  Wait if a writer exists.

                if ((this->Value.FetchAdd(Read) & (Wait | Write)) == 0)
                    return;
                //  Attempt to register as a reader.

                this->Value -= Read;
                //  Whoops, failed. Undo damage.
            }
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;
        }

        /**
         *  <summary>Attempts to acquire the lock as a writer.</summary>
         *  <return>True if the acquisition succeeded; false otherwise.</return>
         */
        inline __must_check bool TryAcquireAsWriter() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            uint32_t val = this->Value.Load();

            if ((val >= Write) || !this->Value.CmpXchgStrong(val, val | Write))
                return false;
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;

            return true;
        }

        /**
         *  <summary>Acquires the lock as the writer.</summary>
         */
        inline void AcquireAsWriter() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            uint32_t val = this->Value.Load();

            while (true)
            {
                //  This can re-use the last value loaded in the last iteration.

                if (val < Write)
                {
                    //  No readers or writers!

                    if (this->Value.CmpXchgStrong(val, Write))
                        return;
                    //  Clear wait bit, set write bit.

                    //  At this point, `val` contains the newest value.
                }

                if (0 == (val & Wait))
                    this->Value |= Wait;
                //  Set writer wait bit.

                while ((val = this->Value.Load()) > Wait)
                    asm volatile ( "pause \n\t" : : : "memory" );
                //  Wait for all the readers and writers to hold their horses.
            }
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;
        }

        /**
         *  <summary>Attempts to upgrade a reader to the writer.</summary>
         *  <return>True if the upgrade succeeded; false otherwise.</return>
         */
        inline __must_check bool UpgradeToWriter() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            if (this->Value.TestSet(WriteBit))
                return false;
            //  Fail if another writer is awaiting on this lock.

            this->Value -= Read;
            //  Don't count this as a reader anymore.

            while (this->Value.Load() & ReadMask)
                asm volatile ( "pause \n\t" : : : "memory" );
            //  Wait for readers to clear.
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;

            return true;
        }

        /*  Release Operations  */

        /**
         *  <summary>Releases the lock as a reader.</summary>
         */
        inline void ReleaseAsReader() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            this->Value -= Read;
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_REL;
        }

        /**
         *  <summary>Releases the lock as the writer.</summary>
         */
        inline void ReleaseAsWriter() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            this->Value &= ~Write;
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_REL;
        }

        /**
         *  <summary>Downgrades the writer to a reader.</summary>
         */
        inline void DowngradeToReader() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            this->Value += Read;
            //  Count this as a reader first.

            this->Value &= ~Write;
            //  De-register as writer.
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_REL;
        }

        /**
         *  <summary>Resets the lock.</summary>
         */
        __forceinline void Reset() volatile
        {
            this->Value.Store(0U);
        }

        /*  Properties  */

        /**
         *  <summary>Determines whether there is an active writer or an awaiting writer..</summary>
         *  <return>True if there is an active/awaiting writer; otherwise false.</return>
         */
        __forceinline __must_check bool HasWriter() const volatile
        {
            return (this->Value.Load() & (Wait | Write)) != 0;
        }

        /**
         *  <summary>Gets the number of active readers.</summary>
         *  <return>The number of active readers.</return>
         */
        __forceinline __must_check size_t GetReaderCount() const volatile
        {
            return (this->Value.Load() & ReadMask) >> ReadBit;
        }

    private:
        /*  Fields  */

        Atomic<uint32_t> Value;

#else

        /*  Acquisition Operations  */

        __forceinline __must_check bool TryAcquireAsReader() volatile
        { this->ReaderCount = 1; return true; }

        inline void AcquireAsReader() volatile
        { this->ReaderCount = 1; }

        __forceinline __must_check bool TryAcquireAsWriter() volatile
        { return true; }

        __forceinline void AcquireAsWriter() volatile { }

        __forceinline bool UpgradeToWriter() volatile
        { this->ReaderCount = 0; return true; }

        /*  Release Operations  */

        __forceinline void ReleaseAsReader() volatile
        { this->ReaderCount = 0; }

        __forceinline void ReleaseAsWriter() volatile { }

        __forceinline void DowngradeToReader() volatile
        { this->ReaderCount = 1; }

        __forceinline void Reset() volatile
        { this->ReaderCount = 0; }

        /*  Properties  */

        __forceinline __must_check bool HasWriter() const volatile
        { return false; }

        __forceinline __must_check size_t GetReaderCount() const volatile
        { return this->ReaderCount; }

        /*  Fields  */

    private:

        char ReaderCount;
#endif
    };
}}
