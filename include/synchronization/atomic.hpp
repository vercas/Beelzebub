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

#include <metaprogramming.h>

namespace Beelzebub { namespace Synchronization
{
    enum class MemoryOrder
    {
        Relaxed = __ATOMIC_RELAXED,
        Consume = __ATOMIC_CONSUME,
        Acquire = __ATOMIC_ACQUIRE,
        Release = __ATOMIC_RELEASE,
        AcqRel  = __ATOMIC_ACQ_REL,
        SeqCst  = __ATOMIC_SEQ_CST,
    };

    template<typename T>
    struct Atomic
    {
        /*  Probing  */

        static inline constexpr bool IsAlwaysLockFree(void const * const ptr = nullptr)
        {
            return __atomic_always_lock_free(sizeof(T), ptr);
        }

        inline bool IsLockFree() volatile
        {
            return __atomic_is_lock_free(sizeof(T), &this->InnerValue);
        }

        /*  Constructors  */
     
        Atomic() = default;
        inline constexpr Atomic(T const val) : InnerValue( val ) { }
        Atomic(Atomic const &) = delete;
        Atomic & operator =(Atomic const &) = delete;
        Atomic & operator =(Atomic const &) volatile = delete;

        /*  Load & Store  */

        inline operator T() const volatile
        {
            return this->Load();
        }

        inline T operator =(T const val) volatile
        {
            this->Store(val);

            return val;
        }
        inline T operator =(T const val)
        {
            this->Store(val);

            return val;
        }

        inline void Store(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_store_n(&this->InnerValue, val, (int)mo);
        }

        inline T Load(MemoryOrder const mo = MemoryOrder::SeqCst) const volatile
        {
            return __atomic_load_n(&this->InnerValue, (int)mo);
        }

        inline T Xchg(T const other, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_exchange_n(&this->InnerValue, other, (int)mo);
        }
        inline void Xchg(T * other, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_exchange(&this->InnerValue, other, other, (int)mo);
        }

        /*  Compare-Exchange  */

        inline bool CmpXchgWeak(T & expected, T const desired, MemoryOrder const smo, MemoryOrder const fmo) volatile
        {
            return __atomic_compare_exchange_n(&this->InnerValue, &expected, desired, true, (int)smo, (int)fmo);
        }

        inline bool CmpXchgStrong(T & expected, T const desired, MemoryOrder const smo, MemoryOrder const fmo) volatile
        {
            return __atomic_compare_exchange_n(&this->InnerValue, &expected, desired, false, (int)smo, (int)fmo);
        }

        inline bool CmpXchgWeak(T & expected, T const desired, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return this->CmpXchgWeak(expected, desired, mo, mo);
        }

        inline bool CmpXchgStrong(T & expected, T const desired, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return this->CmpXchgStrong(expected, desired, mo, mo);
        }

        /*  Arithmetic Fetch-Ops  */

        inline T FetchAdd(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_add(&this->InnerValue, val, (int)mo);
        }

        inline T FetchSub(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_sub(&this->InnerValue, val, (int)mo);
        }

        inline T FetchNeg(MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            T tmp = this->Load(mo);

            do { /* nothing */ } while (!this->CmpXchgWeak(tmp, -tmp, mo));
            //  `tmp` should get the latest value with every iteration.

            return tmp;

            //  GCC and the C++ STL do not provide atomic negation. I do.
        }

        /*  Logic Fetch-Ops  */

        inline T FetchAnd(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_and(&this->InnerValue, val, (int)mo);
        }

        inline T FetchOr(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_or(&this->InnerValue, val, (int)mo);
        }

        inline T FetchXor(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_xor(&this->InnerValue, val, (int)mo);
        }

        inline T FetchNot(MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            T tmp = this->Load(mo);

            do { /* nothing */ } while (!this->CmpXchgWeak(tmp, ~tmp, mo));
            //  `tmp` should get the latest value with every iteration.

            return tmp;

            //  GCC and the C++ STL do not provide atomic binary complement. I do.
        }

        /*  Special Ops  */

#ifdef __BEELZEBUB__ARCH_X86
    #ifdef __GCC_ASM_FLAG_OUTPUTS__
        inline bool TestSet(size_t const bit, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            bool res;

            asm volatile("lock bts %[bit], %[dst] \n\t"
                        : [dst]"+m"(this->InnerValue), "=@ccc"(res)
                        : [bit]"Jr"(bit)
                        : "cc" );

            return res;
        }

        inline bool TestClear(size_t const bit, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            bool res;

            asm volatile("lock btr %[bit], %[dst] \n\t"
                        : [dst]"+m"(this->InnerValue), "=@ccc"(res)
                        : [bit]"Jr"(bit)
                        : "cc" );

            return res;
        }

        inline bool TestComplement(size_t const bit, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            bool res;

            asm volatile("lock btc %[bit], %[dst] \n\t"
                        : [dst]"+m"(this->InnerValue), "=@ccc"(res)
                        : [bit]"Jr"(bit)
                        : "cc" );

            return res;
        }
    #else
        inline bool TestSet(size_t const bit, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            bool res;

            asm volatile("lock bts %[bit], %[dst] \n\t"
                        "setcb %b[res] \n\t"
                        : [dst]"+m"(this->InnerValue), [res]"=qm"(res)
                        : [bit]"Jr"(bit)
                        : "cc" );

            return res;
        }

        inline bool TestClear(size_t const bit, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            bool res;

            asm volatile("lock btr %[bit], %[dst] \n\t"
                        "setcb %b[res] \n\t"
                        : [dst]"+m"(this->InnerValue), [res]"=qm"(res)
                        : [bit]"Jr"(bit)
                        : "cc" );

            return res;
        }

        inline bool TestComplement(size_t const bit, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            bool res;

            asm volatile("lock btc %[bit], %[dst] \n\t"
                        "setcb %b[res] \n\t"
                        : [dst]"+m"(this->InnerValue), [res]"=qm"(res)
                        : [bit]"Jr"(bit)
                        : "cc" );

            return res;
        }
    #endif
#endif

        /*  Arithmetic Op-Fetches  */

        inline T operator +=(T const other) volatile
        {
            return __atomic_add_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        inline T operator -=(T const other) volatile
        {
            return __atomic_sub_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        inline T operator ++() volatile
        {   //  Prefix operator.
            return __atomic_add_fetch(&this->InnerValue, 1, (int)MemoryOrder::SeqCst);
        }

        inline T operator --() volatile
        {   //  Prefix operator.
            return __atomic_sub_fetch(&this->InnerValue, 1, (int)MemoryOrder::SeqCst);
        }

        inline T operator ++(int) volatile
        {   //  Postfix/suffix operator.
            return this->FetchAdd(1);
        }

        inline T operator --(int) volatile
        {   //  Postfix/suffix operator.
            return this->FetchSub(1);
        }

        inline T operator -() const volatile
        {   //  Unary minus.
            return -(this->Load());
        }

        inline T NegFetch() volatile
        {
            T tmp = this->Load(MemoryOrder::SeqCst), neg;

            do { neg = -tmp; } while (!this->CmpXchgWeak(tmp, neg, MemoryOrder::SeqCst));
            //  `tmp` should get the latest value with every iteration.

            return neg;

            //  GCC and the C++ STL do not provide atomic negation. I do.
        }

        /*  Logic Op-Fetches  */

        inline T operator &=(T const other) volatile
        {
            return __atomic_and_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        inline T operator |=(T const other) volatile
        {
            return __atomic_or_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        inline T operator ^=(T const other) volatile
        {
            return __atomic_xor_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        inline T operator ~() const volatile
        {
            return -(this->Load());
        }

        inline T NotFetch() volatile
        {
            T tmp = this->Load(MemoryOrder::SeqCst), com;

            do { com = ~tmp; } while (!this->CmpXchgWeak(tmp, com, MemoryOrder::SeqCst));
            //  `tmp` should get the latest value with every iteration.

            return com;

            //  GCC and the C++ STL do not provide atomic negation. I do. I do...
        }

        /*  Fields  */

        T InnerValue;
    };

    template<typename T>
    struct Atomic<T *>
    {
        /*  Probing  */

        static inline constexpr bool IsAlwaysLockFree()
        {
            return __atomic_always_lock_free(sizeof(T *), 0);
        }

        inline bool IsLockFree() volatile
        {
            return __atomic_is_lock_free(sizeof(T *), &this->InnerValue);
        }

        /*  Constructors  */
     
        Atomic() = default;
        inline constexpr Atomic(T const * const val) : InnerValue( val ) { }
        Atomic(Atomic const &) = delete;
        Atomic & operator =(Atomic const &) = delete;
        Atomic & operator =(Atomic const &) volatile = delete;

        /*  Load & Store  */

        inline operator T * () const volatile
        {
            return this->Load();
        }

        inline T * operator =(T const * const val) volatile
        {
            this->Store(val);

            return val;
        }
        inline T * operator =(T const * const val)
        {
            this->Store(val);

            return val;
        }

        inline void Store(T const * const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_store_n(&this->InnerValue, val, (int)mo);
        }

        inline T * Load(MemoryOrder const mo = MemoryOrder::SeqCst) const volatile
        {
            return __atomic_load_n(&this->InnerValue, (int)mo);
        }

        inline T * Xchg(T const * const other, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_exchange_n(&this->InnerValue, other, (int)mo);
        }
        inline void Xchg(T const * * other, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_exchange(&this->InnerValue, other, other, (int)mo);
        }

        /*  Compare-Exchange  */

        inline bool CmpXchgWeak(T const * const & expected, T * const desired, MemoryOrder const smo, MemoryOrder const fmo) volatile
        {
            return __atomic_compare_exchange_n(&this->InnerValue, &expected, desired, true, (int)smo, (int)fmo);
        }

        inline bool CmpXchgStrong(T const * const & expected, T * const desired, MemoryOrder const smo, MemoryOrder const fmo) volatile
        {
            return __atomic_compare_exchange_n(&this->InnerValue, &expected, desired, false, (int)smo, (int)fmo);
        }

        inline bool CmpXchgWeak(T const * const & expected, T * const desired, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return this->CmpXchgWeak(expected, desired, mo, mo);
        }

        inline bool CmpXchgStrong(T const * const & expected, T * const desired, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return this->CmpXchgStrong(expected, desired, mo, mo);
        }

        /*  Fetch-Ops  */

        inline T * FetchAdd(ptrdiff_t const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_add(&this->InnerValue, val * sizeof(T), (int)mo);
        }

        inline T * FetchSub(ptrdiff_t const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_sub(&this->InnerValue, val * sizeof(T), (int)mo);
        }

        /*  Op-Fetches / Operators  */

        inline T * operator +=(ptrdiff_t const other) volatile
        {
            return __atomic_add_fetch(&this->InnerValue, other * sizeof(T), (int)MemoryOrder::SeqCst);
        }

        inline T * operator -=(ptrdiff_t const other) volatile
        {
            return __atomic_sub_fetch(&this->InnerValue, other * sizeof(T), (int)MemoryOrder::SeqCst);
        }

        inline T * operator ++() volatile
        {   //  Prefix operator.
            return __atomic_add_fetch(&this->InnerValue, sizeof(T), (int)MemoryOrder::SeqCst);
        }

        inline T * operator --() volatile
        {   //  Prefix operator.
            return __atomic_sub_fetch(&this->InnerValue, sizeof(T), (int)MemoryOrder::SeqCst);
        }

        inline T * operator ++(int) volatile
        {   //  Postfix/suffix operator.
            return this->FetchAdd(1);
        }

        inline T * operator --(int) volatile
        {   //  Postfix/suffix operator.
            return this->FetchSub(1);
        }

        /*  Fields  */

        T * InnerValue;
    };

    template<>
    struct Atomic<bool>
    {
        /*  Probing  */

        static inline constexpr bool IsAlwaysLockFree(void const * const ptr = nullptr)
        {
            return __atomic_always_lock_free(sizeof(bool), ptr);
        }

        inline bool IsLockFree() volatile
        {
            return __atomic_is_lock_free(sizeof(bool), &this->InnerValue);
        }

        /*  Constructors  */
     
        Atomic() = default;
        inline constexpr Atomic(bool const val) : InnerValue( val ) { }
        Atomic(Atomic const &) = delete;
        Atomic & operator =(Atomic const &) = delete;
        Atomic & operator =(Atomic const &) volatile = delete;

        /*  Load & Store  */

        inline operator bool() const volatile
        {
            return this->Load();
        }

        inline bool operator =(bool const val) volatile
        {
            this->Store(val);

            return val;
        }
        inline bool operator =(bool const val)
        {
            this->Store(val);

            return val;
        }

        inline void Store(bool const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_store_n(&this->InnerValue, val, (int)mo);
        }

        inline bool Load(MemoryOrder const mo = MemoryOrder::SeqCst) const volatile
        {
            return __atomic_load_n(&this->InnerValue, (int)mo);
        }

        inline bool Xchg(bool const other, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_exchange_n(&this->InnerValue, other, (int)mo);
        }
        inline void Xchg(bool * other, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_exchange(&this->InnerValue, other, other, (int)mo);
        }

        /*  Compare-Exchange  */

        inline bool CmpXchgWeak(bool & expected, bool const desired, MemoryOrder const smo, MemoryOrder const fmo) volatile
        {
            return __atomic_compare_exchange_n(&this->InnerValue, &expected, desired, true, (int)smo, (int)fmo);
        }

        inline bool CmpXchgStrong(bool & expected, bool const desired, MemoryOrder const smo, MemoryOrder const fmo) volatile
        {
            return __atomic_compare_exchange_n(&this->InnerValue, &expected, desired, false, (int)smo, (int)fmo);
        }

        inline bool CmpXchgWeak(bool & expected, bool const desired, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return this->CmpXchgWeak(expected, desired, mo, mo);
        }

        inline bool CmpXchgStrong(bool & expected, bool const desired, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return this->CmpXchgStrong(expected, desired, mo, mo);
        }

        /*  Specific Operations  */

        inline bool TestSet(MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_test_and_set(&this->InnerValue, (int)mo);
        }

        inline void Clear(MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_clear(&this->InnerValue, (int)mo);
        }

        /*  Logical Fetch-Ops  */

        inline bool FetchNot(MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_xor(&this->InnerValue, true, (int)mo);
            //  If your booleans are anything but the compiler's definition of true or false,
            //  it's not my business.
        }

        /*  Logical Op-Fetches  */

        inline bool operator &=(bool const other) volatile
        {
            return __atomic_and_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        inline bool operator |=(bool const other) volatile
        {
            return __atomic_or_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        inline bool operator !() const volatile
        {
            return !(this->Load());
        }

        inline bool NotFetch(MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_xor_fetch(&this->InnerValue, true, (int)mo);
            //  If your booleans are anything but the compiler's definition of true or false,
            //  it's not my business.
        }

        /*  Fields  */

        bool InnerValue;
    };

    typedef Atomic<int8_t>     AtomicInt8;
    typedef Atomic<int16_t>    AtomicInt16;
    typedef Atomic<int32_t>    AtomicInt32;
    typedef Atomic<int64_t>    AtomicInt64;
    typedef Atomic<uint8_t>    AtomicUInt8;
    typedef Atomic<uint16_t>   AtomicUInt16;
    typedef Atomic<uint32_t>   AtomicUInt32;
    typedef Atomic<uint64_t>   AtomicUInt64;
    typedef Atomic<bool>       AtomicBool;
    typedef Atomic<float>      AtomicFloat;
    typedef Atomic<float>      AtomicSingle;
    typedef Atomic<double>     AtomicDouble;
    typedef Atomic<intptr_t>   AtomicIntPtr;
    typedef Atomic<uintptr_t>  AtomicUIntPtr;
    typedef Atomic<size_t>     AtomicSize;
    typedef Atomic<ptrdiff_t>  AtomicPtrDiff;
}}
