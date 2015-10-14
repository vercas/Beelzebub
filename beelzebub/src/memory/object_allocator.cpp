/**
 *  The object allocator uses a bold strategy to achieve multiprocessing safety.
 *  
 *  First of all, there's the object pool.
 *  All of its properties are going to be read and changed while it's locked.
 *  This includes linkage, counters and free object indexes.
 *  When a method tries to work with a pool
 *  
 *  Then, there's the 
 */

#include <memory/object_allocator.hpp>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;

/****************************
    ObjectAllocator class
****************************/

/*  Constructors  */

ObjectAllocator::ObjectAllocator(size_t const objectSize, size_t const objectAlignment, AcquirePoolFunc acquirer, EnlargePoolFunc enlarger, ReleasePoolFunc releaser, bool const canReleaseAll)
    : AcquirePool (acquirer)
    , EnlargePool(enlarger)
    , ReleasePool(releaser)
    , ObjectSize(RoundUp(Minimum(objectSize, sizeof(FreeObject)), objectAlignment))
    , HeaderSize(RoundUp(sizeof(ObjectPool), RoundUp(Minimum(objectSize, sizeof(FreeObject)), objectAlignment)))
    , FirstPool(nullptr)
    , LastPool(nullptr)
    , LinkageLock()
    , Capacity(0)
    , FreeCount(0)
    , PoolCount(0)
    , CanReleaseAllPools(canReleaseAll)
{
    //  As you can see, at least a FreeObject must fit in the object size.
    //  Also, only the alignment of the 
}

/*  Methods  */

Handle ObjectAllocator::AllocateObject(void * & result, size_t estimatedLeft)
{
    Handle res;
    ObjectAllocatorLockCookie volatile cookie;

    ObjectPool * first = nullptr, * last = nullptr, * current = nullptr, * justAllocated = nullptr;

    //  First, let's make sure there is a first pool.
firstPoolCheck:

    if unlikely(this->FirstPool == nullptr)
    {
        cookie = this->LinkageLock.Acquire();

        if unlikely(this->FirstPool != nullptr)
        {   //  Maybe another core allocated the first pool in the meantime.
            this->LinkageLock.Release(cookie);  //  Release lock.

            goto firstPoolCheck;                //  Check again.
        }

        res = this->AcquirePool(this->ObjectSize, this->HeaderSize, Minimum(estimatedLeft, 1), justAllocated);

        if (!res.IsOkayResult())
        {
            this->LinkageLock.Release(cookie);

            return res;
        }

        assert(justAllocated != nullptr
            , "Object allocator %Xp apparently successfully acquired a pool (%H), which appears to be null!"
            , this, res);

        ++this->PoolCount;
        //  We've got an extra pool!

        this->FirstPool = this->LastPool = justAllocated->Next = first = current = justAllocated;
        //  Four fields with the same value... Eh.

        current->PropertiesLock.SimplyAcquire();

        this->LinkageLock.SimplyRelease();
    }
    else
    {
        first = current = this->FirstPool;

        cookie = current->PropertiesLock.Acquire();
    }

poolLoop:

    do
    {
        if (current->FreeCount == 0)
        {
            //  If the current one's full, we get the next.

            ObjectPool * temp = current->Next;

            if (temp == first)
            {
                //  THIS IS THE END! (of the chain)

                current->PropertiesLock.Release(cookie);
                //  Release the current pool and restore interrupts.

                break;
                //  Go to end of the loop.
            }

            //  Otherwise...

            temp->PropertiesLock.SimplyAcquire();
            current->PropertiesLock.SimplyRelease();
            //  Lock the next, release the current.

            current = temp;
        }
        else
        {
            obj_ind_t freeCount = --current->FreeCount;

            FreeObject * obj = current->GetFirstFreeObject(this->ObjectSize, this->HeaderSize);
            current->FirstFreeObject = obj->Next;

            //  Don't unlock. All allocators should wait for this pool to
            //  enlarge.

            if unlikely(freeCount == 0)
            {
                this->EnlargePool(this->ObjectSize, this->HeaderSize, estimatedLeft, current);
                //  This function will release the lock on the object pool as soon as it can.
                //  Also, its return value is not really relevant right now. If it fails,
                //  there's nothing to do... This method call already succeeded. Next one will
                //  have to do something else.
            }
            else
            {
                current->PropertiesLock.Release(cookie);
            }

            result = obj;

            return HandleResult::Okay;
        }
    } while (current != first);
    //  Yes, the condition here is redundant. But it exists just so the compiler doesn't think this loops forever.
    //  Not implying that the compiler would think that, but I've met my fair share of compiler bugs which lead
    //  to me building up this paranoia.

    if (last != nullptr)
    {
        //  Okay, so, for some irrelevant reason, allocation failed in the newly-created pool.
        //  In this case, the function state is reset.

        last = justAllocated = nullptr;
        //  So a new one can be allocated.

        first = current = this->FirstPool;
        //  We start from the beginning.

        cookie = current->PropertiesLock.Acquire();
        //  Get a cookie from the jar.

        goto poolLoop;
        //  Restart.
    }

    //  Okay, so, if this point is reached, it means no free allocator was found...

    last = this->LastPool;
    //  We store this to look for changes.

    if (this->AcquisitionLock.TryAcquire(cookie))
    {
        //  If the lock can be acquired, then this core/thread is the one which must allocate the space.
        //  The rest will await nicely.

        res = this->AcquirePool(this->ObjectSize, this->HeaderSize, Minimum(estimatedLeft, 1), justAllocated);

        if (!res.IsOkayResult())
        {
            this->AcquisitionLock.Release(cookie);

            return res.WithPreppendedResult(HandleResult::ObjaPoolsExhausted);
        }

        assert(justAllocated != nullptr
            , "Object allocator %Xp apparently successfully acquired a pool (%H), which appears to be null!"
            , this, res);

        ++this->PoolCount;
        //  We've got an extra pool!

        last->PropertiesLock.SimplyAcquire();

        this->LastPool = last->Next = justAllocated;

        this->AcquisitionLock.SimplyRelease();
        last->PropertiesLock.Release(cookie);
        //  I release the acquisition lock first so all awaiting cores/threads can go on ASAP.
    }
    else
    {
        //  If the lock is already acquired, it means that another core/thread is attempting to acquire a pool.
        //  Thus, we await.

        this->AcquisitionLock.Await();

        ObjectPool * volatile newLast = this->LastPool;

        if unlikely(last == newLast)
            return HandleResult::ObjaPoolsExhausted;
        else
        {
            //  Okay, so a new allocator has been created while waiting.
            //  Now we prepare the method state to look at this new pool.

            current = newLast;
            //  `first` need not be changed, because `last->Next` == `first`.

            cookie = current->PropertiesLock.Acquire();
            //  We need to lock this new allocator.

            goto poolLoop;
        }
    }

    return res;
}

Handle ObjectAllocator::DeallocateObject(void const * const object)
{
    //  Important note on what would seem like retardery at first sight:
    //  I keep the "previous" pool locked so its `Next` pool can be changed.
    //  I release that lock ASAP.

    Handle res;
    ObjectAllocatorLockCookie volatile cookie;

    ObjectPool * first = nullptr, * current = nullptr, * previous = nullptr;

    if (this->CanReleaseAllPools)
        cookie = this->LinkageLock.Acquire();

    if unlikely(this->FirstPool == nullptr)
    {
        if (this->CanReleaseAllPools)
            this->LinkageLock.Release(cookie);

        return HandleResult::ArgumentOutOfRange;
        //  Aye. If there are no allocators, the object does not belong to this allocator.
    }

    first = current = this->FirstPool;

    if (this->CanReleaseAllPools)
        current->PropertiesLock.SimplyAcquire();    //  Cookie's already in use.
    else
        cookie = current->PropertiesLock.Acquire();

    do
    {
        if (current->Contains((uintptr_t)object, this->ObjectSize, this->HeaderSize))
        {
            if (current->FreeCount > 1)
            {
                //  Free count greater than one means that the pool won't require removal
                //  after this object is deallocated. Therefore, we can release the
                //  previous pool or the linkage chain.

                if (previous != nullptr)
                    previous->PropertiesLock.SimplyRelease();
                else if (this->CanReleaseAllPools)
                    this->LinkageLock.SimplyRelease();
            }
            else
            {
                //  Well, there's no point in updating the pool now, wasting precious CPU cycles on cache misses and
                //  memory loads.

                if (previous != nullptr || this->CanReleaseAllPools)
                {
                    //  So, if there is a previois pool, or all pools can be released
                    res = this->ReleasePool(this->ObjectSize, this->HeaderSize, current);

                    if (res.IsOkayResult())
                    {
                        --this->PoolCount;
                        //  We've got an extra pool!

                        return HandleResult::Okay;
                    }
                }
            }
        }
        else
        {
            //  If the current one doesn't contain the object, try the next.

            ObjectPool * temp = current->Next;

            if (temp == first)
            {
                //  THIS IS THE END! (of the chain)

                if (previous != nullptr)
                    previous->PropertiesLock.SimplyRelease();
                else if (this->CanReleaseAllPools)
                    this->LinkageLock.SimplyRelease();
                //  First release the previous, if any, or the linkage chain.

                current->PropertiesLock.Release(cookie);
                //  Release the current pool and restore interrupts.

                break;
                //  Go to end of the loop.
            }

            //  Otherwise...

            temp->PropertiesLock.SimplyAcquire();
            //  Lock the next, keep the current locked.

            if (previous != nullptr)
                previous->PropertiesLock.SimplyRelease();
            else if (this->CanReleaseAllPools)
                this->LinkageLock.SimplyRelease();
            //  And release the previous, if any, otherwise the linkage chain.

            previous = current;
            current = temp;
        }
    } while (current != first);

    return HandleResult::ArgumentOutOfRange;
    //  If this point is reached, it means the target object is outside of this allocator's
    //  pools.
}
