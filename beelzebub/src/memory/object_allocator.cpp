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
    , ObjectSize(RoundUp(Maximum(objectSize, sizeof(FreeObject)), objectAlignment))
    , HeaderSize(RoundUp(sizeof(ObjectPool), RoundUp(Maximum(objectSize, sizeof(FreeObject)), objectAlignment)))
    , FirstPool(nullptr)
    , LastPool(nullptr)
    , LinkageLock()
    , AcquisitionLock()
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

    ObjectPool * first = nullptr, * volatile last = nullptr, * current = nullptr, * justAllocated = nullptr;

    //  First, let's make sure there is a first pool.
firstPoolCheck:

    if unlikely(this->FirstPool == nullptr)
    {
        cookie = this->LinkageLock.Acquire();

        if unlikely(this->FirstPool != nullptr)
        {
            //  Maybe another core allocated the first pool in the meantime.

            this->LinkageLock.Release(cookie);  //  Release lock.

            goto firstPoolCheck;                //  Check again.
        }

        res = this->AcquirePool(this->ObjectSize, this->HeaderSize, Minimum(estimatedLeft, 2), justAllocated);
        //  A minimum of two is given here so the current allocation can happen
        //  and the next crawler can attempt to enlarge the pool if needed.

        if (!res.IsOkayResult())
        {
            this->LinkageLock.Release(cookie);

            //  Failed to acquire. Oh, well...

            return res;
        }

        assert(justAllocated != nullptr
            , "Object allocator %Xp apparently successfully acquired a pool (%H), which appears to be null!"
            , this, res);

        /*msg("~~ ACQUIRED FIRST POOL %Xp WITH FC=%u4, cap=%u4 ~~%n"
            , justAllocated
            , justAllocated->FreeCount, justAllocated->Capacity);//*/
        COMPILER_MEMORY_BARRIER();

        ++this->PoolCount;
        this->Capacity += justAllocated->Capacity;
        this->FreeCount += justAllocated->FreeCount;
        //  There's a new pool!

        this->FirstPool = this->LastPool = justAllocated->Next = first = current = justAllocated;
        //  Four fields with the same value... Eh.

        current->PropertiesLock.SimplyAcquire();

        this->LinkageLock.SimplyRelease();

        /*msg("~~ ACQUIRED FIRST POOL %Xp WITH FC=%u4, cap=%u4 ~~%n"
            , justAllocated
            , justAllocated->FreeCount, justAllocated->Capacity);//*/
        COMPILER_MEMORY_BARRIER();
    }
    else
    {
        first = current = this->FirstPool;

        cookie = current->PropertiesLock.Acquire();
    }

poolLoop:

    do
    {
        /*msg("~~ CHECKING POOL %Xp WITH FC=%u4, cap=%u4 ~~%n"
            , current, current->FreeCount, current->Capacity);//*/
        COMPILER_MEMORY_BARRIER();

        if (current->FreeCount == 0)
        {
            //  If the current one's full, go get the next.

            ObjectPool * temp = current->Next;

            if (temp == first)
            {
                //  THIS IS THE END! (of the chain)

                current->PropertiesLock.Release(cookie);
                //  Release the current pool and restore interrupts.

                /*msg("~~ REACHED END OF POOL CHAIN ~~%n");//*/

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
            COMPILER_MEMORY_BARRIER();

            obj_ind_t freeCount = --current->FreeCount;

            FreeObject * obj = current->GetFirstFreeObject(this->ObjectSize, this->HeaderSize);
            current->FirstFreeObject = obj->Next;

            //  Don't unlock. All allocators should wait for this pool to
            //  enlarge.

            if unlikely(freeCount == 0)
            {
                current->LastFreeObject = obj_ind_invalid;
                //  There's no last free object anymoar!

                obj_ind_t const oldCapacity = current->Capacity;

                this->EnlargePool(this->ObjectSize, this->HeaderSize, estimatedLeft, current);
                //  Its return value is not really relevant right now. If it fails,
                //  there's nothing to do... This method call already succeeded. Next one will
                //  have to do something else.

                if (current->Capacity != oldCapacity)
                {
                    //  This means the pool was enlarged. Under no circumstances
                    //  should it be shrunk.

                    this->Capacity += current->Capacity - oldCapacity;
                    this->FreeCount += current->FreeCount;
                    //  The free count was 0 before enlarging.

                    //  Also, this part is done under a lock to prevent the ABA
                    //  problem. Perhaps, between unlocking this pool and executing
                    //  this block, the core/thread was interrupted long/often enough
                    //  to allow another one to fill this pool to capacity.
                    //  Thus, both increments above would have wrong values!
                }
                /*else
                    msg("~~ POOL %Xp WAS NOT ENLARGED! ~~%n", current);//*/
            }

            current->PropertiesLock.Release(cookie);

            result = obj;
            --this->FreeCount;
            //  Book-keeping.

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
        //  Start from the beginning.

        cookie = current->PropertiesLock.Acquire();
        //  Get a cookie from the jar.

        goto poolLoop;
        //  Restart.
    }

    //  Okay, so, if this point is reached, it means no free allocator was found...

    last = this->LastPool;
    //  Used to look for changes.

    if (this->AcquisitionLock.TryAcquire(cookie))
    {
        //  If the lock can be acquired, then this core/thread is the one which must allocate the space.
        //  The rest will await nicely.

        res = this->AcquirePool(this->ObjectSize, this->HeaderSize, Minimum(estimatedLeft, 2), justAllocated);
        //  A minimum of two is given here so the current allocation can happen
        //  and the next crawler can attempt to enlarge the pool if needed.

        if (!res.IsOkayResult())
        {
            this->AcquisitionLock.Release(cookie);

            return res.WithPreppendedResult(HandleResult::ObjaPoolsExhausted);
        }

        assert(justAllocated != nullptr
            , "Object allocator %Xp apparently successfully acquired a pool (%H), which appears to be null!"
            , this, res);

        /*msg("~~ ALLOCATED EXTRA POOL %Xp ~~%n", justAllocated);//*/
        COMPILER_MEMORY_BARRIER();

        ++this->PoolCount;
        this->Capacity += justAllocated->Capacity;
        //  Got a new pool!

        FreeObject * obj;
        result = obj = justAllocated->GetFirstFreeObject(this->ObjectSize, this->HeaderSize);
        justAllocated->FirstFreeObject = obj->Next;
        //  This allocator gets the first object as a reward for taking its time
        //  to allocate a new pool.

        this->FreeCount += --justAllocated->FreeCount;
        //  More book-keeping...

        last->PropertiesLock.SimplyAcquire();

        ObjectPool * oldNext = last->Next;
        last->Next = justAllocated;
        justAllocated->Next = oldNext;

        COMPILER_MEMORY_BARRIER();
        //  Gotta make sure that the last pool is set after the ex-last is
        //  modified.

        this->LastPool = justAllocated;

        this->AcquisitionLock.SimplyRelease();
        last->PropertiesLock.Release(cookie);
        //  I release the acquisition lock first so all awaiting cores/threads can go on ASAP.
    }
    else
    {
        //  If the lock is already acquired, it means that another core/thread is attempting to acquire a pool.
        //  Therefore, it awaits.

        this->AcquisitionLock.Await();

        ObjectPool * volatile newLast = this->LastPool;

        if unlikely(last == newLast)
            return HandleResult::ObjaPoolsExhausted;
        else
        {
            //  Okay, so a new allocator has been created while waiting.
            //  Now the method state is prepared to look at this new pool.

            current = newLast;
            //  `first` need not be changed, because `last->Next` == `first`.

            cookie = current->PropertiesLock.Acquire();
            //  The new pool needs to be locked.

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
    obj_ind_t ind = obj_ind_invalid;

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
        if (current->Contains((uintptr_t)object, ind, this->ObjectSize, this->HeaderSize))
        {
            obj_ind_t currentCapacity = current->Capacity;
            obj_ind_t const freeCount = current->FreeCount;

            if likely(currentCapacity - freeCount > 1)
            {
                //  Busy count greater than one means that the pool won't require removal
                //  after this object is deallocated. Therefore, the previous pool or the
                //  linkage chain can be released.

                if (previous != nullptr)
                    previous->PropertiesLock.SimplyRelease();
                else if (this->CanReleaseAllPools)
                    this->LinkageLock.SimplyRelease();

                //  This will proceed to actual removal.
            }
            else
            {
                //  Well, there's no point in updating the pool now, wasting precious CPU cycles on cache misses and
                //  memory loads.

                if (previous != nullptr || this->CanReleaseAllPools || this->PoolCount > 1)
                {
                    //  So, if there was a previous pool, this one can be removed.
                    //  If all pools can be released, this one can be.
                    //  If there's more than one pool, what the heck, let's remove it.

                    //  If any other pool is in the process of being removed, its
                    //  previous pool will be locked, which means that it cannot be
                    //  removed at the same time. Thus, there's no ABA problem.

                    ObjectPool * const next = unlikely(current == current->Next) ? nullptr : current->Next;
                    //  If it's the only pool, next is null!

                    res = this->ReleasePool(this->ObjectSize, this->HeaderSize, current);
                    //  This method call could very well have just reduced the pool,
                    //  if it failed to deallocate it for some reason. If it returns
                    //  okay, it simply tells the allocator to unplug this pool.
                    //  Otherwise, the function should make sure whatever's left of
                    //  the pool is usable and the allocator will simply adjust.
                    //  The capacity and free count should be reset by the function.

                    if likely(res.IsOkayResult())
                    {
                        //  So, the pool is now removed. Let's update stuff.

                        if (previous != nullptr)
                        {
                            //  If there was a previous pool, then `current->Next`
                            //  is not equal to `current`.

                            previous->Next = next;
                            
                            previous->PropertiesLock.Release(cookie);
                        }
                        else
                        {
                            if (next != current)
                                this->FirstPool = next;
                            else
                            {
                                if (this->LastPool == current)
                                    this->LastPool = nullptr;

                                this->FirstPool = nullptr;
                            }

                            this->LinkageLock.Release(cookie);
                        }

                        --this->PoolCount;
                        this->Capacity -= currentCapacity;
                        this->FreeCount -= freeCount;

                        return HandleResult::Okay;
                    }
                    else
                    {
                        //  So, for whatever reason, the removing failed.
                        //  Now this is practically a fresh pool.

                        if (previous != nullptr)
                            previous->PropertiesLock.Release(cookie);
                        else
                            this->LinkageLock.Release(cookie);
                        //  These need not be locked anymore for what they protect
                        //  shall not be modified.

                        obj_ind_t const capDiff = currentCapacity - current->Capacity;
                        ssize_t const freeDiff = (ssize_t)freeCount - (ssize_t)current->FreeCount;

                        this->Capacity -= capDiff;
                        this->FreeCount -= freeDiff;
                        //  The sign shouldn't matter, rite? At worst this is -1.

                        current->PropertiesLock.Release(cookie);

                        return HandleResult::Okay;
                    }

                    //  Reaching this point means standard-issue removal.
                }
            }

            FreeObject * const freeObject = (FreeObject *)(uintptr_t)object;
            freeObject->Next = current->FirstFreeObject;

            current->FirstFreeObject = ind;
            ++current->FreeCount;

            current->PropertiesLock.Release(cookie);
            //  Release the current pool and restore interrupts.

            ++this->FreeCount;
            //  I hate book-keeping.

            return HandleResult::Okay;
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
