#pragma once

#if   defined(__BEELZEBUB__TEST_OBJA)
    #define private public
    #define protected public
#endif

#include <synchronization/atomic.hpp>
#include <debug.hpp>
#include <handles.h>

#if   defined(__BEELZEBUB_KERNEL)
    #if   defined(__BEELZEBUB_SETTINGS_OBJA_RRSPINLOCK)
        #include <synchronization/round_robin_spinlock.hpp>
    #endif

    #include <synchronization/spinlock_uninterruptible.hpp>
#endif

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;

namespace Beelzebub { namespace Memory
{
#if defined(__BEELZEBUB_KERNEL)
    #if   defined(__BEELZEBUB_SETTINGS_OBJA_RRSPINLOCK)
        typedef LOCK NAME GOES HERE ObjectAllocatorLock;
    #else
        typedef SpinlockUninterruptible<> ObjectAllocatorLock;
    #endif

    typedef int_cookie_t ObjectAllocatorLockCookie;
#else
    //  To do: userland will require a mutex here.
#endif

    /** <summary>Type used to represent the index of an object.</summary> */
    typedef uint32_t obj_ind_t;

    /** <summary>A set of predefined object index values with special meanings.</summary> */
    enum obj_ind_special : obj_ind_t
    {
        obj_ind_invalid = 0xFFFFFFFFU,
    };

    /**
     *  <summary>Represents the contents of a free object in the object pool.</summary>
     */
    struct FreeObject
    {
        obj_ind_t Next;
    };
    //  GCC makes this 8 bytes on AMD64. Have it your way, [female dog].

    /**
     *  <summary>Represents an area of memory where objects can be allocated.</summary>
     */
    struct ObjectPool
    {
        /*  Fields  */

        obj_ind_t volatile FreeCount;
        obj_ind_t Capacity;  //  This should help determine the size of the pool.
        obj_ind_t FirstFreeObject;      //  Used by the allocator.
        obj_ind_t LastFreeObject;       //  Used for enlarging, mainly.
        //  These should be creating an alignment of up to 16 if needed.

        ObjectPool * Next;
        //ObjectPool * Previous;

        //ObjectPoolStatus Status;
        ObjectAllocatorLock PropertiesLock;

        /*  Constructors  */

        __bland inline ObjectPool()
            : FreeCount( 0)
            , Capacity(0)
            , FirstFreeObject(obj_ind_invalid)
            , LastFreeObject(obj_ind_invalid)
            , Next(nullptr)
            , PropertiesLock()
        {

        }

        ObjectPool(ObjectPool const &) = delete;
        ObjectPool & operator =(const ObjectPool &) = delete;
        //  Aye - no copying.

        /*  Methods  */

        __bland inline bool Contains(const uintptr_t object, const size_t objectSize, const size_t headerSize) const
        {
            uintptr_t start = ((uintptr_t)this) + headerSize;
            return object >= start && object <= (start + (this->Capacity - 1) * objectSize);
        }

        __bland inline bool Contains(const uintptr_t object, obj_ind_t & ind, const size_t objectSize, const size_t headerSize) const
        {
            uintptr_t const start = ((uintptr_t)this) + headerSize;

            if likely(object >= start)
            {
                uintptr_t const offset = (object - start) / objectSize;

                if likely(offset < (uintptr_t)this->Capacity)
                {
                    ind = (obj_ind_t)offset;
                    return true;
                }
            }

            ind = obj_ind_invalid;
            return false;
        }

        __bland inline obj_ind_t IndexOf(const uintptr_t object, const size_t objectSize, const size_t headerSize) const
        {
            return (obj_ind_t)((object - headerSize - ((uintptr_t)this)) / objectSize);
        }

        __bland inline FreeObject * GetFirstFreeObject(const size_t objectSize, const size_t headerSize) const
        {
            return (FreeObject *)(uintptr_t)((uintptr_t)this + headerSize + this->FirstFreeObject * objectSize);
        }
        __bland inline FreeObject * GetLastFreeObject(const size_t objectSize, const size_t headerSize) const
        {
            return (FreeObject *)(uintptr_t)((uintptr_t)this + headerSize + this->LastFreeObject * objectSize);
        }
    };

    typedef Handle (*AcquirePoolFunc)(size_t objectSize, size_t headerSize, size_t minimumObjects, ObjectPool * & result);
    //  The acquiring code need not lock the object pool.
    typedef Handle (*EnlargePoolFunc)(size_t objectSize, size_t headerSize, size_t minimumExtraObjects, ObjectPool * pool);
    //  The enlarging code will operate with a fully-locked pool and will leave it as such.
    typedef Handle (*ReleasePoolFunc)(size_t objectSize, size_t headerSize, ObjectPool * pool);
    //  The release code need not (un)lock the object poool.

    //  Why so many parameters? So the provider can make the best decisions! :)

    /**
     *  <summary>Manages pools of fixed-size objects.</summary>
     */
    class ObjectAllocator
    {
    public:

        /*  Constructors  */

        __bland inline ObjectAllocator()
            : AcquirePool( nullptr)
            , EnlargePool(nullptr)
            , ReleasePool(nullptr)
            , ObjectSize(0)
            , HeaderSize(0)
            , FirstPool(nullptr)
            , LastPool(nullptr)
            , LinkageLock()
            , AcquisitionLock()
            , Capacity(0)
            , FreeCount(0)
            , PoolCount(0)
            , CanReleaseAllPools(true)
        {
            //  This constructor is required because of the const fields.
        }
             
        ObjectAllocator(ObjectAllocator const &) = delete;
        ObjectAllocator & operator =(const ObjectAllocator &) = delete;

        __bland ObjectAllocator(size_t const objectSize, size_t const objectAlignment, AcquirePoolFunc acquirer, EnlargePoolFunc enlarger, ReleasePoolFunc releaser, bool const canReleaseAll = false);

        /*  Methods  */

        template<typename T>
        __bland inline Handle AllocateObject(T * & result, size_t estimatedLeft = 1)
        {
            assert_or(sizeof(T) <= this->ObjectSize
                , "Attempted to allocate an object of size %us on allocator "
                  "%Xp with size %us!"
                , sizeof(T), this, this->ObjectSize)
            {
                return HandleResult::ArgumentTemplateInvalid;
            }
            
            //  Yes, smaller objects will be accepted.

            void * pRes;

            Handle hRes = this->AllocateObject(pRes, estimatedLeft);

            result = (T *)pRes;

            return hRes;
        }

        __hot __bland __noinline Handle AllocateObject(void * & result, size_t estimatedLeft = 1);
        __hot __bland __noinline Handle DeallocateObject(void const * const object);
        //  These are complex methods and GCC will not be intimidated.

        /*  Properties  */

        __bland inline size_t GetCapacity() const
        {
            return this->Capacity.Load();
        }

        __bland inline size_t GetFreeCount() const
        {
            return this->FreeCount.Load();
        }

        /*  Parameters  */

    private:

        AcquirePoolFunc AcquirePool;
        EnlargePoolFunc EnlargePool;
        ReleasePoolFunc ReleasePool;

    public:

        size_t const ObjectSize;
        size_t const HeaderSize;

    private:

        /*  Links  */

        ObjectPool * volatile FirstPool;
        ObjectPool * volatile LastPool;

        ObjectAllocatorLock LinkageLock;
        SpinlockUninterruptible<> AcquisitionLock;

        /*  Stats  */

        Atomic<size_t> Capacity;
        Atomic<size_t> FreeCount;
        Atomic<size_t> PoolCount;

        /*  Config  */

        bool const CanReleaseAllPools;
    };
}}

#if   defined(__BEELZEBUB__TEST_OBJA)
    #undef private
    #undef protected
#endif
