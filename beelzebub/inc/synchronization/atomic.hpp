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

        static __bland inline constexpr bool IsAlwaysLockFree(void const * const ptr = nullptr)
        {
            return __atomic_always_lock_free(sizeof(T), ptr);
        }

        __bland inline bool IsLockFree() volatile
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

        __bland inline operator T() const volatile
        {
            return this->Load();
        }

        __bland inline T operator =(T const val) volatile
        {
            this->Store(val);

            return val;
        }
        __bland inline T operator =(T const val)
        {
            this->Store(val);

            return val;
        }

        __bland inline void Store(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_store_n(&this->InnerValue, val, (int)mo);
        }

        __bland inline T Load(MemoryOrder const mo = MemoryOrder::SeqCst) const volatile
        {
            return __atomic_load_n(&this->InnerValue, (int)mo);
        }

        __bland inline T Xchg(T other, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_exchange_n(&this->InnerValue, other, (int)mo);
        }
        __bland inline void Xchg(T * other, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_exchange(&this->InnerValue, other, (int)mo);
        }

        /*  Compare-Exchange  */

        __bland inline bool CmpXchgWeak(T & expected, T const desired, MemoryOrder const smo, MemoryOrder const fmo) volatile
        {
            return __atomic_compare_exchange_n(&this->InnerValue, &expected, desired, true, (int)smo, (int)fmo);
        }

        __bland inline bool CmpXchgStrong(T & expected, T const desired, MemoryOrder const smo, MemoryOrder const fmo) volatile
        {
            return __atomic_compare_exchange_n(&this->InnerValue, &expected, desired, false, (int)smo, (int)fmo);
        }

        __bland inline bool CmpXchgWeak(T & expected, T const desired, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return this->CmpXchgWeak(expected, desired, mo, mo);
        }

        __bland inline bool CmpXchgStrong(T & expected, T const desired, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return this->CmpXchgStrong(expected, desired, mo, mo);
        }

        /*  Fetch-Ops  */

        __bland inline T FetchAdd(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_add(&this->InnerValue, val, (int)mo);
        }

        __bland inline T FetchSub(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_sub(&this->InnerValue, val, (int)mo);
        }

        __bland inline T FetchAnd(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_and(&this->InnerValue, val, (int)mo);
        }

        __bland inline T FetchOr(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_or(&this->InnerValue, val, (int)mo);
        }

        __bland inline T FetchXor(T const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_xor(&this->InnerValue, val, (int)mo);
        }

        /*  Op-Fetches / Operators  */

        __bland inline T operator +=(T const other) volatile
        {
            return __atomic_add_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        __bland inline T operator -=(T const other) volatile
        {
            return __atomic_sub_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        __bland inline T operator &=(T const other) volatile
        {
            return __atomic_and_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        __bland inline T operator |=(T const other) volatile
        {
            return __atomic_or_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        __bland inline T operator ^=(T const other) volatile
        {
            return __atomic_xor_fetch(&this->InnerValue, other, (int)MemoryOrder::SeqCst);
        }

        __bland inline T operator ++() volatile
        {   //  Prefix operator.
            return __atomic_add_fetch(&this->InnerValue, 1, (int)MemoryOrder::SeqCst);
        }

        __bland inline T operator --() volatile
        {   //  Prefix operator.
            return __atomic_sub_fetch(&this->InnerValue, 1, (int)MemoryOrder::SeqCst);
        }

        __bland inline T operator ++(int) volatile
        {   //  Postfix/suffix operator.
            return this->FetchAdd(1);
        }

        __bland inline T operator --(int) volatile
        {   //  Postfix/suffix operator.
            return this->FetchSub(1);
        }

        /*  Fields  */

        T InnerValue;
    };

    template<typename T>
    struct Atomic<T *>
    {
        /*  Probing  */

        static __bland inline constexpr bool IsAlwaysLockFree()
        {
            return __atomic_always_lock_free(sizeof(T *), 0);
        }

        __bland inline bool IsLockFree() volatile
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

        __bland inline operator T * () const volatile
        {
            return this->Load();
        }

        __bland inline T * operator =(T * const val) volatile
        {
            this->Store(val);

            return val;
        }
        __bland inline T * operator =(T * const val)
        {
            this->Store(val);

            return val;
        }

        __bland inline void Store(T * const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_store_n(&this->InnerValue, val, (int)mo);
        }

        __bland inline T * Load(MemoryOrder const mo = MemoryOrder::SeqCst) const volatile
        {
            return __atomic_load_n(&this->InnerValue, (int)mo);
        }

        __bland inline T * Xchg(T * other, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_exchange_n(&this->InnerValue, other, (int)mo);
        }
        __bland inline void Xchg(T * * other, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            __atomic_exchange(&this->InnerValue, other, (int)mo);
        }

        /*  Compare-Exchange  */

        __bland inline bool CmpXchgWeak(T * const & expected, T * const desired, MemoryOrder const smo, MemoryOrder const fmo) volatile
        {
            return __atomic_compare_exchange_n(&this->InnerValue, &expected, desired, true, (int)smo, (int)fmo);
        }

        __bland inline bool CmpXchgStrong(T * const & expected, T * const desired, MemoryOrder const smo, MemoryOrder const fmo) volatile
        {
            return __atomic_compare_exchange_n(&this->InnerValue, &expected, desired, false, (int)smo, (int)fmo);
        }

        __bland inline bool CmpXchgWeak(T * const & expected, T * const desired, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return this->CmpXchgWeak(expected, desired, mo, mo);
        }

        __bland inline bool CmpXchgStrong(T * const & expected, T * const desired, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return this->CmpXchgStrong(expected, desired, mo, mo);
        }

        /*  Fetch-Ops  */

        __bland inline T * FetchAdd(ptrdiff_t const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_add(&this->InnerValue, val * sizeof(T), (int)mo);
        }

        __bland inline T * FetchSub(ptrdiff_t const val, MemoryOrder const mo = MemoryOrder::SeqCst) volatile
        {
            return __atomic_fetch_sub(&this->InnerValue, val * sizeof(T), (int)mo);
        }

        /*  Op-Fetches / Operators  */

        __bland inline T * operator +=(ptrdiff_t const other) volatile
        {
            return __atomic_add_fetch(&this->InnerValue, other * sizeof(T), (int)MemoryOrder::SeqCst);
        }

        __bland inline T * operator -=(ptrdiff_t const other) volatile
        {
            return __atomic_sub_fetch(&this->InnerValue, other * sizeof(T), (int)MemoryOrder::SeqCst);
        }

        __bland inline T * operator ++() volatile
        {   //  Prefix operator.
            return __atomic_add_fetch(&this->InnerValue, sizeof(T), (int)MemoryOrder::SeqCst);
        }

        __bland inline T * operator --() volatile
        {   //  Prefix operator.
            return __atomic_sub_fetch(&this->InnerValue, sizeof(T), (int)MemoryOrder::SeqCst);
        }

        __bland inline T * operator ++(int) volatile
        {   //  Postfix/suffix operator.
            return this->FetchAdd(1);
        }

        __bland inline T * operator --(int) volatile
        {   //  Postfix/suffix operator.
            return this->FetchSub(1);
        }

        /*  Fields  */

        T * InnerValue;
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
    typedef Atomic<double>     AtomicDouble;
    typedef Atomic<intptr_t>   AtomicIntPtr;
    typedef Atomic<uintptr_t>  AtomicUIntPtr;
    typedef Atomic<size_t>     AtomicSize;
    typedef Atomic<ptrdiff_t>  AtomicPtrDiff;
}}
