/*
    Copyright (c) 2017 Alexandru-Mihai Maftei. All rights reserved.


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
    union rwticketlock_t
    {
        uint64_t Whole;
        uint32_t LowerHalf;

        __extension__ struct
        {
            uint16_t WritersTail, ReadersTail, Head;
        };
    };

    struct RwTicketLock
    {
        /*  Constructor(s)  */

        RwTicketLock() = default;

        RwTicketLock(RwTicketLock const &) = delete;
        RwTicketLock & operator =(RwTicketLock const &) = delete;
        RwTicketLock(RwTicketLock &&) = delete;
        RwTicketLock & operator =(RwTicketLock &&) = delete;

#ifdef __BEELZEBUB_SETTINGS_SMP
    #ifdef __BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS
        __noinline __must_check bool TryAcquireAsReader() volatile;
        __noinline void AcquireAsReader() volatile;
        __noinline __must_check bool TryAcquireAsWriter() volatile;
        __noinline void AcquireAsWriter() volatile;
        __noinline __must_check bool UpgradeToWriter() volatile;
        __noinline void ReleaseAsReader() volatile;
        __noinline void ReleaseAsWriter() volatile;
        __noinline void DowngradeToReader() volatile;
        __noinline void Reset() volatile;
        __noinline __must_check bool HasWriter() const volatile;
        __noinline __must_check size_t GetReaderCount() const volatile;
    #else

        /*  Acquisition Operations  */

        /**
         *  <summary>Attempts to acquire the lock as a reader.</summary>
         *  <return>True if the acquisition succeeded; false otherwise.</return>
         */
        inline __must_check bool TryAcquireAsReader() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            uint64_t const me = this->Value.Head;
            uint64_t const write = this->Value.WritersTail;
            uint64_t const cmpnew = ((me + 1) << 32) + ((me + 1) << 16) + write;
            uint64_t cmp = (me << 32) + (me << 16) + write;

            if (!__atomic_compare_exchange_n(&(this->Value.Whole), &cmp, cmpnew, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                return false;
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
            uint16_t const me = __atomic_fetch_add(&(this->Value.Head), 1, __ATOMIC_ACQUIRE);

            uint16_t diff;

            while ((diff = me - this->Value.ReadersTail) != 0)
                do DO_NOTHING(); while (--diff > 0);

            COMPILER_MEMORY_BARRIER();

            ++this->Value.ReadersTail;
            //  This need not be atomic because the readers enter one-by-one.
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
            uint64_t const me = this->Value.Head;
            uint64_t const read = this->Value.ReadersTail << 16;
            uint64_t const cmpnew = ((me + 1) << 32) + read + me;
            uint64_t cmp = (me << 32) + read + me;

            if (!__atomic_compare_exchange_n(&(this->Value.Whole), &cmp, cmpnew, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
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
            uint16_t const me = __atomic_fetch_add(&(this->Value.Head), 1, __ATOMIC_ACQUIRE);

            uint16_t diff;

            while ((diff = me - this->Value.WritersTail) != 0)
                do DO_NOTHING(); while (--diff > 0);
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_ACQ;
        }

        /**
         *  <summary>Attempts to upgrade a reader to the writer.</summary>
         *  <return>True if the upgrade succeeded; false otherwise.</return>
         */
        template<bool weak = false>
        inline __must_check bool UpgradeToWriter() volatile
        {
            COMPILER_MEMORY_BARRIER();

        op_start:
            do
            {
                rwticketlock_t const cur { this->Value.Whole };

                if (cur.ReadersTail - cur.WritersTail != 1)
                    return false;
                //  Can't wait for other readers, another might be trying to
                //  upgrade as well.

                //  This can only work if this is the only reader.

                rwticketlock_t old = cur;
                rwticketlock_t const des {{cur.WritersTail, cur.ReadersTail - 1, cur.Head}};

                //  This succeeds if it can push back the readers tail.

                if (!__atomic_compare_exchange_n(&(this->Value.Whole), &(old.Whole), des.Whole, weak, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                {
                    if (weak || old.Head == cur.Head)
                        return false;
                    else
                        continue;
                }
            } while (!weak);
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
            __atomic_add_fetch(&(this->Value.WritersTail), 1, __ATOMIC_RELEASE);
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
            rwticketlock_t temp { __atomic_load_n(&(this->Value.Whole), __ATOMIC_RELAXED) };

            ++temp.WritersTail;
            ++temp.ReadersTail;

            __atomic_store_n(&(this->Value.LowerHalf), temp.LowerHalf, __ATOMIC_RELEASE);
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
            ++this->Value.ReadersTail;
            //  It's this simple, yes.
        op_end:

            COMPILER_MEMORY_BARRIER();
            ANNOTATE_LOCK_OPERATION_REL;
        }

        /**
         *  <summary>Resets the lock.</summary>
         */
        __forceinline void Reset() volatile
        {
            this->Value.Whole = 0;
        }

        /*  Properties  */

        /**
         *  <summary>Determines whether there is an active writer or an awaiting writer..</summary>
         *  <return>True if there is an active/awaiting writer; otherwise false.</return>
         *  <remarks>Due to design, this method is inaccurate.</remarks>
         */
        __forceinline __must_check bool HasWriter() const volatile
        {
            return this->Value.Head > this->Value.ReadersTail;
        }

        /**
         *  <summary>Gets the number of active readers.</summary>
         *  <return>The number of active readers.</return>
         */
        __forceinline __must_check size_t GetReaderCount() const volatile
        {
            return this->Value.ReadersTail - this->Value.WritersTail;
        }
    #endif

    private:
        /*  Fields  */

        rwticketlock_t Value;
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

        uint64_t ReaderCount;
#endif
    };
}}
