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

#include <beel/metaprogramming.h>

namespace Beelzebub::Utils
{
    template<typename T>
    struct ObjectPool
    {
        /*  Statics  */

#ifdef __BEELZEBUB__ARCH_AMD64
        static constexpr uint64_t const PointerMask = 0x7FFFFFFFFFFFFFFFUL;
#else
#error TODO
        static constexpr uint64_t const PointerMask = 0x00000000FFFFFFFFUL;
#endif

        static constexpr uint64_t const ValueMask   = 0x7FFFFFFFFFFFFFFFUL;
        static constexpr uint64_t const BusyMask    = 0x8000000000000000UL;

        static constexpr uint64_t const NoNext      = 0x7FFFFFFFFFFFFFFFUL;
        static constexpr uint64_t const NotFree     = 0x7FFFFFFFFFFFFFFEUL;

        static constexpr uint64_t const Unallocated = 0xFFFFFFFFFFFFFFFFUL;
        static constexpr uint64_t const Deallocated = 0xFFFFFFFFFFFFFFFEUL;

        struct Entry
        {
            uint64_t Value;

            inline bool IsBusy() const { return 0 != (this->Value & BusyMask); }
            inline bool IsFree() const { return 0 == (this->Value & BusyMask); }

            inline bool IsAwaitingValue() const { return this->Value == 0xFFFFFFFFFFFFFFFFUL; }

            inline uint64_t GetNext() const { return this->Value & ValueMask; }
            
            inline T * GetPointer() const
            {
#ifdef __BEELZEBUB__ARCH_AMD64
                return 0 != (this->Value & 0x0000800000000000UL)
                    ? this->Value | 0xFFFF000000000000UL
                    : this->Value & 0x0000FFFFFFFFFFFFUL;
#else
                return reinterpret_cast<T *>(this->Value & PointerMask);
#endif
            }

            inline bool Acquire(uint64_t & next)
            {
                uint64_t old = __atomic_load_n(&(this->Value), __ATOMIC_ACQUIRE);

                do
                {
                    if (0 != (old & BusyMask))
                        return false;
                    //  Busy bit is 1 means this cannot be acquired.
                } while (!__atomic_compare_exchange_n(&(this->Value), &old, 0xFFFFFFFFFFFFFFFFUL, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED);

                next = old & ValueMask;

                return true;
            }

            inline bool SetPointer(T * val)
            {
                uint64_t const newVal = reinterpret_cast<uint64_t>(val) | BusyMask;
                uint64_t old = __atomic_load_n(&(this->Value), __ATOMIC_ACQUIRE);

                if (old != 0xFFFFFFFFFFFFFFFFUL)
                    return false;

                return __atomic_compare_exchange_n(&(this->Value), &old, newVal, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
            }

            inline bool UnsetPointer(T * & val)
            {
                uint64_t old = __atomic_load_n(&(this->Value), __ATOMIC_ACQUIRE);

                if (old == 0xFFFFFFFFFFFFFFFFUL || 0 == (old & BusyMask))
                    return false;

                return __atomic_compare_exchange_n(&(this->Value), &old, Deallocated, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
            }
        };

        /*  Constructors  */

        inline ObjectPool()
            : Head( NoNext), Padding1(), Tail(NoNext), Padding2()
            , Entries(nullptr), Capacity(0)
        {
            
        }

        inline ObjectPool(void * storage, size_t cap)
            : Head( NoNext), Padding1(), Tail(NoNext), Padding2()
            , Entries(reinterpret_cast<Entry *>(storage)), Capacity(cap)
        {
            
        }

        /*  Actions  */

        inline uint64_t Pop()
        {
            uint64_t head;

            do
            {
                head = __atomic_load_n(&(this->Head), __ATOMIC_ACQUIRE);

                if (head == NoNext)
                    break;
            } while (!this->Entries[head].Acquire(this->Head));
            
            return head;
        }

        inline bool SetPointer(uint64_t id, T * val)
        {
            if (id >= this->Capacity)
                return false;

            return this->Entries[id].SetPointer(val);
        }

        inline bool UnsetPointer(uint64_t id, T * & val)
        {
            if (id >= this->Capacity)
                return false;

            return this->Entries[id].UnsetPointer(val);
        }

    private:
        /*  Fields  */

        uint64_t Head;
        uint8_t Padding1[__BEELZEBUB__CACHE_LINE_SIZE - sizeof(uint64_t)];

        uint64_t Tail;
        uint8_t Padding2[__BEELZEBUB__CACHE_LINE_SIZE - sizeof(uint64_t)];

        Entry * Entries;
        size_t Capacity;
    };
}
