/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

/*  Note that the implementation of this header is architecture-specific.  */

#pragma once

#include <beel/sync/smp.lock.hpp>

namespace Beelzebub::Utils
{
    template<typename T>
    struct ObjectPool
    {
        /*  Statics  */

#ifdef __BEELZEBUB__ARCH_AMD64
        static constexpr uintptr_t const ValueMask   = 0xFFFFFFFFFFFFFFFEUL;
        static constexpr uintptr_t const ValueShift  = 1;
        static constexpr uintptr_t const BusyMask    = 0x0000000000000001UL;

        static constexpr uintptr_t const NoNext      = 0xFFFFFFFFFFFFFFFEUL;
        static constexpr uintptr_t const NotFree     = 0xFFFFFFFFFFFFFFFCUL;

        static constexpr uintptr_t const Unallocated = 0xFFFFFFFFFFFFFFFFUL;
        static constexpr uintptr_t const Deallocated = 0xFFFFFFFFFFFFFFFDUL;
#else
#error TODO
        static constexpr uintptr_t const ValueMask   = 0xFFFFFFFEUL;
        static constexpr uintptr_t const ValueShift  = 1;
        static constexpr uintptr_t const BusyMask    = 0x00000001UL;

        static constexpr uintptr_t const NoNext      = 0xFFFFFFFEUL;
        static constexpr uintptr_t const NotFree     = 0xFFFFFFFCUL;

        static constexpr uintptr_t const Unallocated = 0xFFFFFFFFUL;
        static constexpr uintptr_t const Deallocated = 0xFFFFFFFAUL;
#endif

        /*  Constructors  */

        inline ObjectPool()
            : Head( NoNext), Tail(NoNext), Entries(nullptr), Capacity(0), Lock()
        {
            
        }

        inline ObjectPool(void * storage, size_t cap)
            : Head( 0), Tail(cap - 1)
            , Entries(reinterpret_cast<Entry *>(storage)), Capacity(cap)
            , Lock()
        {
            for (uintptr_t i = 0; i < cap - 1; ++i)
                this->Entries[i] = (i + 1) << ValueShift;

            this->Entries[cap - 1] = NoNext;
        }

        /*  Actions  */

        inline uintptr_t Acquire()
        {
            uintptr_t head;

            withLock (this->Lock)
            {
                head = this->Head;

                if (head != NoNext)
                {
                    if (this->Entries[head] == NoNext)
                        this->Head = this->Tail = NoNext;
                    else
                        this->Head = this->Entries[head] >> ValueShift;

                    this->Entries[head] = Unallocated;
                }
            }

            return head;
        }

        inline uintptr_t Acquire(T const * val)
        {
            uintptr_t head;

            withLock (this->Lock)
            {
                head = this->Head;

                if (head != NoNext)
                {
                    if (this->Entries[head] == NoNext)
                        this->Head = this->Tail = NoNext;
                    else
                        this->Head = this->Entries[head] >> ValueShift;

                    this->Entries[head] = reinterpret_cast<uintptr_t>(val) | BusyMask;
                }
            }

            return head;
        }

        inline bool SetPointer(uintptr_t id, T const * val)
        {
            if (id >= this->Capacity || 0 != (reinterpret_cast<uintptr_t>(val) & ValueMask))
                return false;

            // bool set = true;

            // withLock (this->Lock)
            // {
            //     if (this->Entries[id] == Unallocated)
            //         this->Entries[id] = reinterpret_cast<uintptr_t>(val) | BusyMask;
            //     else
            //         set = false;
            // }

            // return set;

            uintptr_t const newVal = reinterpret_cast<uintptr_t>(val) | BusyMask;
            uintptr_t old = Unallocated;

            return __atomic_compare_exchange_n(this->Entries + id, &old, newVal, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED);
        }

        inline bool UnsetPointer(uintptr_t id, T * & val)
        {
            if (id >= this->Capacity)
                return false;

            // bool set = true;

            // withLock (this->Lock)
            // {
            //     if (0 == (this->Entries[id] & BusyMask) || this->Entries[id] == Unallocated)
            //         set = false;
            //     else
            //     {
            //         val = reinterpret_cast<T *>(this->Entries[id] & ValueMask);
            //         this->Entries[id] = Deallocated;
            //     }
            // }

            // return set;

            uintptr_t old = __atomic_load_n(this->Entries + id, __ATOMIC_ACQUIRE);

            if (old == Unallocated || 0 == (old & BusyMask))
                return false;

            return __atomic_compare_exchange_n(this->Entries + id, &old, Deallocated, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
        }

        inline T * Resolve(uintptr_t id)
        {
            if (id >= this->Capacity)
                return nullptr;

            uintptr_t val = __atomic_load_n(this->Entries + id, __ATOMIC_ACQUIRE);

            if (val == Unallocated || 0 == (val & BusyMask))
                return nullptr;

            return reinterpret_cast<T *>(val & ValueMask);
        }

        inline bool Release(uintptr_t id)
        {
            if (id >= this->Capacity)
                return false;

            uintptr_t old = __atomic_load_n(this->Entries + id, __ATOMIC_ACQUIRE);

            if (0 == (old & BusyMask))
                return false;

            if (!__atomic_compare_exchange_n(this->Entries + id, &old, NoNext, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
            {
                // TODO: Catch fire?
                return false;
            }

            withLock (this->Lock)
            {
                if (this->Tail == NoNext)
                    this->Head = this->Tail = id;
                else
                {
                    this->Entries[this->Tail] = id << ValueShift;
                    this->Tail = id;
                }
            }

            return true;
        }

    private:
        /*  Fields  */

        uintptr_t Head;
        uintptr_t Tail;

        uintptr_t * Entries;
        size_t Capacity;

        SmpLockUni Lock;
    };
}
