/****************************
    OBJA_ALOC_TYPE class
****************************/

/*  Constructors  */

OBJA_ALOC_TYPE::OBJA_ALOC_TYPE(size_t const objectSize, size_t const objectAlignment
    , AcquirePoolFunc acquirer, EnlargePoolFunc enlarger, ReleasePoolFunc releaser
    , bool const canReleaseAll, size_t const busyBit)
    : AcquirePool (acquirer)
    , EnlargePool(enlarger)
    , ReleasePool(releaser)
    , ObjectSize(RoundUp(Maximum(objectSize, sizeof(FreeObject)), objectAlignment))
    , HeaderSize(RoundUp(sizeof(OBJA_POOL_TYPE), RoundUp(Maximum(objectSize, sizeof(FreeObject)), objectAlignment)))
    , FirstPool(nullptr)
#ifdef OBJA_MULTICONSUMER
    , LinkageLock()
#endif
    , Capacity(0)
    , FreeCount(0)
    , PoolCount(0)
    , CanReleaseAllPools(canReleaseAll)
    , BusyBit(busyBit)
{
    //  As you can see, at least a FreeObject must fit in the object size.
    //  Also, only the alignment of the 
}

/*  Methods  */

Handle OBJA_ALOC_TYPE::AllocateObject(void * & result, size_t estimatedLeft)
{
    Handle res;

#ifdef OBJA_MULTICONSUMER
    OBJA_COOK_TYPE volatile cookie;
#endif

    OBJA_POOL_TYPE * first, * current;
    ObjectPoolBase * justAllocated = nullptr;

    //  First, let's make sure there is a first pool.
firstPoolCheck:
#ifdef OBJA_MULTICONSUMER
    cookie = this->LinkageLock.Acquire();
#endif

    //  Yes, those two are set there, even to nullptr again.
    if unlikely((first = current = this->FirstPool) == nullptr)
    {
        if unlikely(this->FirstPool != nullptr)
        {
            //  Maybe another core allocated the first pool in the meantime.

#ifdef OBJA_MULTICONSUMER
            this->LinkageLock.Release(cookie);  //  Release lock.
#endif

            goto firstPoolCheck;                //  Check again.
        }

        res = this->AcquirePool(this->ObjectSize, this->HeaderSize, Minimum(estimatedLeft, 2), justAllocated);
        //  A minimum of two is given here so the current allocation can happen
        //  and the next crawler can attempt to enlarge the pool if needed.

        if (!res.IsOkayResult())
        {
#ifdef OBJA_MULTICONSUMER
            this->LinkageLock.Release(cookie);
#endif

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

#ifdef OBJA_MULTICONSUMER
        reinterpret_cast<OBJA_POOL_TYPE *>(justAllocated)->PropertiesLock.SimplyRelease();
#endif

        ++this->PoolCount;
        this->Capacity += justAllocated->Capacity;
        this->FreeCount += justAllocated->FreeCount;
        //  There's a new pool!

        this->FirstPool = first = current = reinterpret_cast<OBJA_POOL_TYPE *>(justAllocated->Next = justAllocated);
        //  Four fields with the same value... Eh.
    }

#ifdef OBJA_MULTICONSUMER
    current->PropertiesLock.SimplyAcquire();
    this->LinkageLock.SimplyRelease();
#endif

    COMPILER_MEMORY_BARRIER();

    do
    {
        /*msg("~~ CHECKING POOL %Xp WITH FC=%u4, cap=%u4 ~~%n"
            , current, current->FreeCount, current->Capacity);//*/
        COMPILER_MEMORY_BARRIER();

        if (current->FreeCount == 0)
        {
            //  If the current one's full, go get the next.

            OBJA_POOL_TYPE * temp = reinterpret_cast<OBJA_POOL_TYPE *>(current->Next);

            if (temp == first)
            {
                //  THIS IS THE END! (of the chain)

                /*msg("~~ REACHED END OF POOL CHAIN ~~%n");//*/

                break;
                //  Go to end of the loop.
            }

            //  Otherwise...

#ifdef OBJA_MULTICONSUMER
            temp->PropertiesLock.SimplyAcquire();
            current->PropertiesLock.SimplyRelease();
            //  Lock the next, release the current.
#endif

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

            if (this->BusyBit < SIZE_MAX)
            {
                uint8_t * const busyByte = (uint8_t *)obj + (this->BusyBit >> 3);
                //  This be the byte containing the busy bit.

                *busyByte |= (1 << (this->BusyBit & 7));
                //  This just sets the busy bit.
            }

#ifdef OBJA_MULTICONSUMER
            current->PropertiesLock.Release(cookie);
#endif

            result = obj;
            --this->FreeCount;
            //  Book-keeping.

            return HandleResult::Okay;
        }
    } while (current != first);
    //  Yes, the condition here is redundant. But it exists just so the compiler doesn't think this loops forever.
    //  Not implying that the compiler would think that, but I've met my fair share of compiler bugs which lead
    //  to me building up this paranoia.

    //  Right here the last pool in the chain is locked and there is no free object
    //  available in any pool.
    //  Last pool need not be removed or tampered with while a new one is created.

    res = this->AcquirePool(this->ObjectSize, this->HeaderSize, Minimum(estimatedLeft, 2), justAllocated);
    //  A minimum of two is given here so the current allocation can happen
    //  and the next crawler can attempt to enlarge the pool if needed.

    if (!res.IsOkayResult())
    {
#ifdef OBJA_MULTICONSUMER
        current->PropertiesLock.Release(cookie);
#endif

        return res.WithPreppendedResult(HandleResult::ObjaPoolsExhausted);
    }

    assert(justAllocated != nullptr
        , "Object allocator %Xp apparently successfully acquired a pool (%H), which appears to be null!"
        , this, res);

    COMPILER_MEMORY_BARRIER();

#ifdef OBJA_MULTICONSUMER
    reinterpret_cast<OBJA_POOL_TYPE *>(justAllocated)->PropertiesLock.SimplyRelease();
#endif

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

    ObjectPoolBase * oldNext = current->Next;
    current->Next = justAllocated;
    justAllocated->Next = oldNext;

    COMPILER_MEMORY_BARRIER();
    //  Gotta make sure that the last pool is set after the ex-last is
    //  modified.

#ifdef OBJA_MULTICONSUMER
    current->PropertiesLock.Release(cookie);
#endif

    return res;
}

Handle OBJA_ALOC_TYPE::DeallocateObject(void * const object)
{
    //  If this object is already freed, report!

    if (this->BusyBit < SIZE_MAX)
    {
        uint8_t const busyByte = *((uint8_t const *)object + (this->BusyBit >> 3));
        //  This is the byte containing the busy bit.

        if (0 == (busyByte & (1 << (this->BusyBit & 7))))
            return HandleResult::ObjaAlreadyFree;

        //  This asynchronous check is performed here even for single-consumer allocators
        //  for the sake of speed - to avoid locking the pools just to do nothing with
        //  them.
    }

    //  Important note on what would seem like retardery at first sight:
    //  I keep the "previous" pool locked so its `Next` pool can be changed.
    //  I release that lock ASAP.

    Handle res;

#ifdef OBJA_MULTICONSUMER
    OBJA_COOK_TYPE volatile cookie;
#endif

    OBJA_POOL_TYPE * first = nullptr, * current = nullptr, * previous = nullptr;
    obj_ind_t ind = obj_ind_invalid;

#ifdef OBJA_MULTICONSUMER
    if (this->CanReleaseAllPools)
        cookie = this->LinkageLock.Acquire();
#endif

    if unlikely(this->FirstPool == nullptr)
    {
#ifdef OBJA_MULTICONSUMER
        if (this->CanReleaseAllPools)
            this->LinkageLock.Release(cookie);
#endif

        return HandleResult::ArgumentOutOfRange;
        //  Aye. If there are no allocators, the object does not belong to this allocator.
    }

    first = current = this->FirstPool;

#ifdef OBJA_MULTICONSUMER
    if (this->CanReleaseAllPools)
        current->PropertiesLock.SimplyAcquire();    //  Cookie's already in use.
    else
        cookie = current->PropertiesLock.Acquire();
#endif

    do
    {
        if (current->Contains((uintptr_t)object, ind, this->ObjectSize, this->HeaderSize))
        {
            obj_ind_t currentCapacity = current->Capacity;
            obj_ind_t const freeCount = current->FreeCount;

            if likely(currentCapacity - freeCount > 1)
            {
#ifdef OBJA_MULTICONSUMER
                //  Busy count greater than one means that the pool won't require removal
                //  after this object is deallocated. Therefore, the previous pool or the
                //  linkage chain can be released.

                if (previous != nullptr)
                    previous->PropertiesLock.SimplyRelease();
                else if (this->CanReleaseAllPools)
                    this->LinkageLock.SimplyRelease();
#endif

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

                    ObjectPoolBase * const next = unlikely(current == current->Next) ? nullptr : current->Next;
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
                            
#ifdef OBJA_MULTICONSUMER
                            previous->PropertiesLock.Release(cookie);
#endif
                        }
                        else
                        {
                            if (next != current)
                                this->FirstPool = reinterpret_cast<OBJA_POOL_TYPE *>(next);
                            else
                                this->FirstPool = nullptr;

#ifdef OBJA_MULTICONSUMER
                            this->LinkageLock.Release(cookie);
#endif
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

#ifdef OBJA_MULTICONSUMER
                        if (previous != nullptr)
                            previous->PropertiesLock.Release(cookie);
                        else
                            this->LinkageLock.Release(cookie);
                        //  These need not be locked anymore for what they protect
                        //  shall not be modified.
#endif

                        obj_ind_t const capDiff = currentCapacity - current->Capacity;
                        ssize_t const freeDiff = (ssize_t)freeCount - (ssize_t)current->FreeCount;

                        this->Capacity -= capDiff;
                        this->FreeCount -= freeDiff;
                        //  The sign shouldn't matter, rite? At worst this is -1.

#ifdef OBJA_MULTICONSUMER
                        current->PropertiesLock.Release(cookie);
#endif

                        return HandleResult::Okay;
                    }

                    //  Reaching this point means standard-issue removal.
                }
            }

            if (this->BusyBit < SIZE_MAX)
            {
                uint8_t * const busyByte = (uint8_t *)object + (this->BusyBit >> 3);
                //  This is the byte containing the free bit.

#ifdef OBJA_MULTICONSUMER
                if (0 == (*busyByte & (1 << (this->BusyBit & 7))))
                {
                    //  And yes, this check is performed again... this time,
                    //  synchronously, to prevent the ABA problem.

                    current->PropertiesLock.Release(cookie);

                    return HandleResult::ObjaAlreadyFree;

                    //  But obviously this only needs to be done if the allocator
                    //  can have multiple consumers.
                }
#endif

                *busyByte &= ~(1 << (this->BusyBit & 7));
                //  This just sets the busy bit.
            }

            FreeObject * const freeObject = (FreeObject *)(uintptr_t)object;
            freeObject->Next = current->FirstFreeObject;

            current->FirstFreeObject = ind;
            ++current->FreeCount;

#ifdef OBJA_MULTICONSUMER
            current->PropertiesLock.Release(cookie);
            //  Release the current pool and restore interrupts.
#endif

            ++this->FreeCount;
            //  I hate book-keeping.

            return HandleResult::Okay;
        }
        else
        {
            //  If the current one doesn't contain the object, try the next.

            OBJA_POOL_TYPE * temp = reinterpret_cast<OBJA_POOL_TYPE *>(current->Next);

            if (temp == first)
            {
                //  THIS IS THE END! (of the chain)

#ifdef OBJA_MULTICONSUMER
                if (previous != nullptr)
                    previous->PropertiesLock.SimplyRelease();
                else if (this->CanReleaseAllPools)
                    this->LinkageLock.SimplyRelease();
                //  First release the previous, if any, or the linkage chain.

                current->PropertiesLock.Release(cookie);
                //  Release the current pool and restore interrupts.
#endif

                break;
                //  Go to end of the loop.
            }

            //  Otherwise...

#ifdef OBJA_MULTICONSUMER
            temp->PropertiesLock.SimplyAcquire();
            //  Lock the next, keep the current locked.

            if (previous != nullptr)
                previous->PropertiesLock.SimplyRelease();
            else if (this->CanReleaseAllPools)
                this->LinkageLock.SimplyRelease();
            //  And release the previous, if any, otherwise the linkage chain.
#endif

            previous = current;
            current = temp;
        }
    } while (current != first);

    return HandleResult::ArgumentOutOfRange;
    //  If this point is reached, it means the target object is outside of this allocator's
    //  pools.
}
