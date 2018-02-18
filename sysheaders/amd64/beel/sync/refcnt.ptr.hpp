/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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
    template<typename T>
    struct RefcntPtr
    {
        /*  Types  */

        typedef uint16_t CountType;

        union Internal
        {
            inline constexpr Internal() : Pointer(nullptr) { }
            inline constexpr explicit Internal(T * val) : Pointer(val) { }
            inline constexpr explicit Internal(uintptr_t val) : Integer(val) { }

            inline constexpr explicit Internal(T * val, uint16_t cnt)
                : Integer((reinterpret_cast<uintptr_t>(val) & 0x0000FFFFFFFFFFFFUL) | (cnt << 48))
            {

            }

            inline constexpr Internal(uint16_t w1, uint16_t w2, uint16_t w3, uint16_t w4)
                : Words({w1, w2, w3, w4})
            {

            }

            T * Pointer;
            uintptr_t Integer;
            uint16_t Words[4];

            inline uint16_t GetRefcnt() const { return this->Words[3]; }

            inline uintptr_t GetUIntPtr() const
            {
                if (this->Integer & 0x0000800000000000UL)
                    return this->Integer | 0xFFFF800000000000UL;
                else
                    return this->Integer & 0x00007FFFFFFFFFFFUL;
            }

            inline T * GetPointer() const
            {
                return reinterpret_cast<T *>(this->GetUIntPtr());
            }
        };

        static_assert(sizeof(Internal) == 8, "Size mismatch.");

        static constexpr Internal const RefcntZero {0};
        static constexpr Internal const RefcntUnit {0, 0, 0, 1};

        /*  Probing  */

        static inline constexpr bool IsAlwaysLockFree(void const * const ptr = nullptr)
        {
            return Atomic<uintptr_t>::IsAlwaysLockFree(ptr);
        }

        inline bool IsLockFree() volatile
        {
            return Atomic<uintptr_t>::IsLockFree();
        }

        /*  Constructors  */
     
        Atomic() = default;
        inline constexpr Atomic(T * const val) : InnerValue(Internal(reinterpret_cast<uintptr_t>(val))) { }
        Atomic(Atomic const &) = delete;
        Atomic & operator =(Atomic const &) = delete;
        Atomic & operator =(Atomic const &) volatile = delete;

        /*  Operations  */

        inline Internal Acquire()
        {
            return Internal(this->InnerValue += RefcntUnit.GetRefcnt());
        }

        inline CountType Release()
        {
            return Internal(this->InnerValue -= RefcntUnit.GetRefcnt()).GetRefcnt();
        }

        inline bool TrySwap(T * const newVal, bool const internal, T * & other)
        {
            CountType const expectedCnt = internal ? 1 : 0;

            Internal const desired {newVal, expectedCnt};
            Internal old;

            do
            {
                old = this->InnerValue.Load();

                if (old.GetRefcnt() != expectedCnt)
                {
                    other = old.GetPointer();

                    return false;
                }
            } while (!this->InnerValue.CmpXchgStrong(old, desired));

            other = old.GetPointer();

            return true;
        }

        inline bool TrySwap(T * const newVal, bool const internal)
        {
            CountType const expectedCnt = internal ? 1 : 0;

            Internal const desired {newVal, expectedCnt};

            do
            {
                Internal old = this->InnerValue.Load();

                if (old.GetRefcnt() != expectedCnt)
                    return false;
            } while (!this->InnerValue.CmpXchgStrong(old, desired));

            return true;
        }

        inline bool CmpXchg(T * & expected, T * const newVal, CountType const cnt)
        {
            Internal old {expected, cnt};
            Internal const desired {newVal, cnt};

            bool const res = this->InnerValue.CmpXchgStrong(old, desired);

            expected = old.GetPointer();

            return res;
        }

        /*  Fields  */

        Atomic<uintptr_t> InnerValue;
    };
#undef CAST
}}
