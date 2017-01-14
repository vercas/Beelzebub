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

#include <beel/sync/rw.spinlock.hpp>

using namespace Beelzebub::Synchronization;

/************************
    RwSpinlock struct
************************/

#ifdef __BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS
    /*  Acquisition Operations  */

    bool RwSpinlock::TryAcquireAsReader() volatile
    {
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

        return true;
    }

    void RwSpinlock::AcquireAsReader() volatile
    {
        while (true)
        {
            while (this->Value.Load() & (Wait | Write))
                DO_NOTHING();
            //  Wait if a writer exists.

            if ((this->Value.FetchAdd(Read) & (Wait | Write)) == 0)
                return;
            //  Attempt to register as a reader.

            this->Value -= Read;
            //  Whoops, failed. Undo damage.
        }
    }

    bool RwSpinlock::TryAcquireAsWriter() volatile
    {
        COMPILER_MEMORY_BARRIER();

        uint32_t val = this->Value.Load();

        if ((val >= Write) || !this->Value.CmpXchgStrong(val, val | Write))
            return false;

        return true;
    }

    void RwSpinlock::AcquireAsWriter() volatile
    {
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
                DO_NOTHING();
            //  Wait for all the readers and writers to hold their horses.
        }
    }

    bool RwSpinlock::UpgradeToWriter() volatile
    {
        if (this->Value.TestSet(WriteBit))
            return false;
        //  Fail if another writer is awaiting on this lock.

        this->Value -= Read;
        //  Don't count this as a reader anymore.

        while (this->Value.Load() & ReadMask)
            DO_NOTHING();
        //  Wait for readers to clear.

        return true;
    }

    /*  Release Operations  */

    void RwSpinlock::ReleaseAsReader() volatile
    {
        this->Value -= Read;
    }

    void RwSpinlock::ReleaseAsWriter() volatile
    {
        this->Value &= ~Write;
    }

    void RwSpinlock::DowngradeToReader() volatile
    {
        this->Value += Read;
        //  Count this as a reader first.

        this->Value &= ~Write;
        //  De-register as writer.
    }

    void RwSpinlock::Reset() volatile
    {
        this->Value.Store(0U);
    }

    /*  Properties  */

    bool RwSpinlock::HasWriter() const volatile
    {
        return (this->Value.Load() & (Wait | Write)) != 0;
    }

    size_t RwSpinlock::GetReaderCount() const volatile
    {
        return (this->Value.Load() & ReadMask) >> ReadBit;
    }
#endif
