#pragma once

#include <metaprogramming.h>
#include <handles.h>
#include <synchronization/spinlock_uninterruptible.hpp>
#include <synchronization/atomic.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;

namespace Beelzebub { namespace Memory
{
#ifdef __BEELZEBUB_KERNEL
    typedef SpinlockUninterruptible<> ObjectAllocatorLock;
    typedef int_cookie_t ObjectAllocatorLockCookie;
#else
    //  To do: userland will require a mutex here.
#endif

    /** <summary>Type used to represent the index of an object.</summary> */
    typedef uint32_t obj_ind_t;

    /**
     *  <summary>Represents the contents of a free object in the object pool.</summary>
     */
    struct FreeObject
    {
        obj_ind_t Next;
    };

    /**
     *  <summary>Represents an area of memory where objects can be allocated.</summary>
     */
    struct ObjectPool
    {
        /*  Fields  */

        obj_ind_t Capacity;  //  This should help determine the size of the pool.
        obj_ind_t volatile FreeCount;
        obj_ind_t FirstFreeObject;      //  Used by the allocator.
        obj_ind_t LastFreeObject;       //  Used for enlarging, mainly.
        //  These should be creating an alignment of up to 16 if needed.

        ObjectPool * Next;
        //ObjectPool * Previous;

        //ObjectPoolStatus Status;
        ObjectAllocatorLock PropertiesLock;

        /*  Constructors  */

        ObjectPool() = default;
        ObjectPool(ObjectPool const &) = delete;
        ObjectPool & operator =(const ObjectPool &) = delete;
        //  Aye - no copying.

        /*  Methods  */

        __bland __forceinline bool Contains(const uintptr_t object, const size_t objectSize, const size_t headerSize) const
        {
            uintptr_t start = ((uintptr_t)this) + headerSize;
            return object >= start && object <= (start + (this->Capacity - 1) * objectSize);
        }

        __bland __forceinline FreeObject * GetFirstFreeObject(const size_t objectSize, const size_t headerSize) const
        {
            return (FreeObject *)(uintptr_t)((uintptr_t)this + headerSize + this->FirstFreeObject * objectSize);
        }
    };

    typedef Handle (*AcquirePoolFunc)(size_t objectSize, size_t headerSize, size_t minimumObjects, ObjectPool * & result);
    //  The acquiring code need not lock the object pool.
    typedef Handle (*EnlargePoolFunc)(size_t objectSize, size_t headerSize, size_t minimumExtraObjects, ObjectPool * pool);
    //  The enlarging code should release the lock on the object pool as soon as it can.
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

        ObjectAllocator() = default;
        ObjectAllocator(ObjectAllocator const &) = delete;
        ObjectAllocator & operator =(const ObjectAllocator &) = delete;

        __bland ObjectAllocator(size_t const objectSize, size_t const objectAlignment, AcquirePoolFunc acquirer, EnlargePoolFunc enlarger, ReleasePoolFunc releaser, bool const canReleaseAll = false);

        /*  Methods  */

        __bland Handle AllocateObject(void * & result, size_t estimatedLeft = 1);
        __bland Handle DeallocateObject(void const * const object);

        /*  Parameters  */

        AcquirePoolFunc AcquirePool;
        EnlargePoolFunc EnlargePool;
        ReleasePoolFunc ReleasePool;

        size_t const ObjectSize;
        size_t const HeaderSize;

        /*  Links  */

        ObjectPool * volatile FirstPool;
        ObjectPool * volatile LastPool;

        ObjectAllocatorLock LinkageLock;
        ObjectAllocatorLock AcquisitionLock;

        /*  Stats  */

        Atomic<size_t> Capacity;
        Atomic<size_t> FreeCount;
        Atomic<size_t> PoolCount;

        /*  Config  */

        bool const CanReleaseAllPools;
    };
}}
