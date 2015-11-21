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

/****************************
    OBJA_ALOC_TYPE class
****************************/

/*  Constructors  */

OBJA_ALOC_TYPE::OBJA_ALOC_TYPE(size_t const objectSize, size_t const objectAlignment
    , AcquirePoolFunc acquirer, EnlargePoolFunc enlarger, ReleasePoolFunc releaser
    , PoolReleaseOptions const releaseOptions, size_t const busyBit, size_t const quota)
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
    , ReleaseOptions(releaseOptions)
    , BusyBit(busyBit)
    , BusyCount(0)
    , Quota(quota)
{
    //  As you can see, at least a FreeObject must fit in the object size.
    //  Also, only the alignment of the object is taken into account.
    //  On platforms that force alignment, it will be enough for FreeObject as well.
    //  On those that don't, the very little usage of the FreeObject will not really
    //  hurt due to unalignment.
}

/*  Methods  */

Handle OBJA_ALOC_TYPE::AllocateObject(void * & result, size_t estimatedLeft)
{
    if (this->BusyCount++ >= this->GetQuota())
    {
        --this->BusyCount;
        //  It was post-incremented. There's no new object.

        return HandleResult::ObjaMaximumCapacity;

        //  This asynchronous check is performed here even for multi-consumer
        //  allocators for the sake of speed - to avoid locking the pools just
        //  to do nothing with them in the end. Synchronous checking is
        //  performed later under the appropriate lock.
    }

    Handle res;

#ifdef OBJA_UNINTERRUPTED
    OBJA_COOK_TYPE volatile cookie;
#endif

    OBJA_POOL_TYPE * current;
    ObjectPoolBase * justAllocated = nullptr;

    //  First, let's make sure there is a first pool.

#ifdef OBJA_UNINTERRUPTED
    cookie = System::Interrupts::PushDisable();
#endif

#ifdef OBJA_MULTICONSUMER
    this->LinkageLock.Acquire();
#endif

    if (this->AcquirePool == nullptr)
    {
#ifdef OBJA_MULTICONSUMER
        this->LinkageLock.Release();
#endif

#ifdef OBJA_UNINTERRUPTED
        System::Interrupts::RestoreState(cookie);
#endif

        return HandleResult::ObjectDisposed;
    }

    //  Note assignment.
    if unlikely((current = this->FirstPool) == nullptr)
    {
        res = this->AcquirePool(this->ObjectSize, this->HeaderSize, Minimum(estimatedLeft, 2), justAllocated);
        //  A minimum of two is given here so the current allocation can happen
        //  and the next crawler can attempt to enlarge the pool if needed.

        if (!res.IsOkayResult())
        {
            //  Failed to acquire. Oh, well...

#ifdef OBJA_MULTICONSUMER
            this->LinkageLock.Release();
#endif

#ifdef OBJA_UNINTERRUPTED
            System::Interrupts::RestoreState(cookie);
#endif

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
        reinterpret_cast<OBJA_POOL_TYPE *>(justAllocated)->PropertiesLock.Release();
#endif

        ++this->PoolCount;
        this->Capacity += justAllocated->Capacity;
        this->FreeCount += justAllocated->FreeCount;
        //  There's a new pool!

        this->FirstPool = current = reinterpret_cast<OBJA_POOL_TYPE *>(justAllocated);
        //  Three fields with the same value... Eh.
        justAllocated->Next = nullptr;
        //  This one's null, heh.
    }

#ifdef OBJA_MULTICONSUMER
    current->PropertiesLock.Acquire();
    this->LinkageLock.Release();
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

            if (temp == nullptr)
                break;
            //  Continue to allocate a pool.

            //  Otherwise...

#ifdef OBJA_MULTICONSUMER
            temp->PropertiesLock.Acquire();
            current->PropertiesLock.Release();
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
                uint8_t * const busyByte = reinterpret_cast<uint8_t *>(obj) + (this->BusyBit >> 3);
                //  This be the byte containing the busy bit.

                *busyByte |= (1 << (this->BusyBit & 7));
                //  This just sets the busy bit.
            }

#ifdef OBJA_MULTICONSUMER
            current->PropertiesLock.Release();
#endif

            result = obj;
            --this->FreeCount;
            //  Book-keeping.

#ifdef OBJA_UNINTERRUPTED
            System::Interrupts::RestoreState(cookie);
#endif

            return HandleResult::Okay;
        }
    } while (current != nullptr);
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
        current->PropertiesLock.Release();
#endif

        --this->BusyCount;
        //  There's not gonna be any new objects under these circumstances. :(

#ifdef OBJA_UNINTERRUPTED
        System::Interrupts::RestoreState(cookie);
#endif

        return res.WithPreppendedResult(HandleResult::ObjaPoolsExhausted);
    }

    assert(justAllocated != nullptr
        , "Object allocator %Xp apparently successfully acquired a pool (%H), which appears to be null!"
        , this, res);

    COMPILER_MEMORY_BARRIER();

#ifdef OBJA_MULTICONSUMER
    reinterpret_cast<OBJA_POOL_TYPE *>(justAllocated)->PropertiesLock.Release();
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

    if (this->BusyBit < SIZE_MAX)
    {
        uint8_t * const busyByte = (uint8_t *)obj + (this->BusyBit >> 3);
        //  This be the byte containing the busy bit.

        *busyByte |= (1 << (this->BusyBit & 7));
        //  This just sets the busy bit.
    }

    COMPILER_MEMORY_BARRIER();
    //  Gotta make sure that the last pool is set after the ex-last is
    //  modified.

#ifdef OBJA_MULTICONSUMER
    current->PropertiesLock.Release();
#endif

#ifdef OBJA_UNINTERRUPTED
    System::Interrupts::RestoreState(cookie);
#endif

    //  Sad note: To increase allocation performance, I could have preppended
    //  the new pool as the first pool, but that would make this code much more
    //  complex and would require more locking.

    return res;
}

Handle OBJA_ALOC_TYPE::DeallocateObject(void * const object)
{
#ifdef OBJA_UNINTERRUPTED
    OBJA_COOK_TYPE volatile cookie = System::Interrupts::PushDisable();
#endif

    //  Even though the code 'til the next lock acquisition consist exclusively
    //  of reading the state and checking conditions, disabling interrupts is
    //  required to make sure that something hasn't changed after it's been read
    //  (ABA problem).

    //  If this object is already freed, report!

    uint8_t * const busyByte = (this->BusyBit < SIZE_MAX)
        ? ((uint8_t *)object + (this->BusyBit >> 3))
        : nullptr;
    //  This is the byte containing the busy bit. It's going to be used at least
    //  twice.

    if (busyByte != nullptr && 0 == (*busyByte & (1 << (this->BusyBit & 7))))
    {
#ifdef OBJA_UNINTERRUPTED
        System::Interrupts::RestoreState(cookie);
#endif

        return HandleResult::ObjaAlreadyFree;

        //  This asynchronous check is performed here even for multi-consumer
        //  allocators for the sake of speed - to avoid locking the pools just
        //  to do nothing with them in the end. Synchronous checking is
        //  performed later under the appropriate lock.
    }

    //  Important note on what would seem silly at first sight:
    //  I keep the "previous" pool locked so its `Next` pool can be changed.
    //  I release that lock ASAP.

    Handle res;

    OBJA_POOL_TYPE * current = nullptr, * previous = nullptr;
    obj_ind_t ind = obj_ind_invalid;

#ifdef OBJA_MULTICONSUMER
    this->LinkageLock.Acquire();
#endif

    if (this->AcquirePool == nullptr)
    {
#ifdef OBJA_MULTICONSUMER
        this->LinkageLock.Release();
#endif

#ifdef OBJA_UNINTERRUPTED
        System::Interrupts::RestoreState(cookie);
#endif

        return HandleResult::ObjectDisposed;
    }

    if unlikely(this->FirstPool == nullptr)
    {
#ifdef OBJA_MULTICONSUMER
        this->LinkageLock.Release();
#endif

#ifdef OBJA_UNINTERRUPTED
        System::Interrupts::RestoreState(cookie);
#endif

        return HandleResult::ArgumentOutOfRange;
        //  Aye. If there are no allocators, the object does not belong to this allocator.
    }

    current = this->FirstPool;

#ifdef OBJA_MULTICONSUMER
    current->PropertiesLock.Acquire();

    bool const canReleaseAny = this->ReleaseOptions != PoolReleaseOptions::NoRelease;

    if (!canReleaseAny)
        this->LinkageLock.Release();
    //  No pools can be released? No need to lock the chain.
#endif

    do
    {
        if (current->Contains((uintptr_t)object, ind, this->ObjectSize, this->HeaderSize))
        {
#ifdef OBJA_MULTICONSUMER
            //  If this is a multi-consumer allocator, this object could've been
            //  freed in the meantime. A check under the containing pool's lock
            //  is synchronous and will make sure there are no races (ABA problem).

            if unlikely(busyByte != nullptr
                && 0 == (*busyByte & (1 << (this->BusyBit & 7))))
            {
                if (canReleaseAny)
                {
                    if (previous != nullptr)
                        previous->PropertiesLock.Release();
                    else
                        this->LinkageLock.Release();
                }

                //  If none can be released, the previous pool or the chain will
                //  not be locked.

                current->PropertiesLock.Release();

                msg("Attempted to deallocate free object %Xp!%n"
                    , object);//*/

                return HandleResult::ObjaAlreadyFree;

                //  But obviously this only needs to be done if the allocator
                //  can have multiple consumers.
            }

            //  Obviously (to me now as well) this check needs to be performed
            //  prior to attempting to remove the pool...

            //  Also, no such (re)check is rquired on single-consumer uninterruptible
            //  allocators.
#endif

            --this->BusyCount;
            //  Do this early. Hoping this will ease the contention on the allocator's
            //  cache line(s). This will delay releasing the previous/linkage lock
            //  a bit but hopefully this won't be that bad.

            obj_ind_t const currentCapacity = current->Capacity;
            obj_ind_t const freeCount = current->FreeCount;

            if unlikely(currentCapacity - freeCount == 1
                && (this->ReleaseOptions == PoolReleaseOptions::ReleaseAll
                    || (this->PoolCount > 1
                        && this->ReleaseOptions == PoolReleaseOptions::KeepOne)))
            {
                //  If the pool's busy count is 1, this is a candidate for removal.
            
                //  So, if all pools can be released, this one can be.
                //  If no pools can be released, this condition will be false.
                //  Otherwise, more than one pool (and the options say keep one)
                //  means this one can be removed.

                //  If any other pool is in the process of being removed, its
                //  previous pool will be locked, which means that it cannot be
                //  removed at the same time. Thus, there's no ABA problem when
                //  it's supposed to keep at least one pool.

                //  Also, `canReleaseAny` is false if this block is executing.

                ObjectPoolBase * const next = current->Next;
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
                    //  So, the pool is now removed. Or to messed up to reuse.
                    //  Let's update stuff.

                    if (previous != nullptr)
                    {
                        previous->Next = next;
                        
#ifdef OBJA_MULTICONSUMER
                        previous->PropertiesLock.Release();
#endif
                    }
                    else
                    {
                        this->FirstPool = reinterpret_cast<OBJA_POOL_TYPE *>(next);

#ifdef OBJA_MULTICONSUMER
                        this->LinkageLock.Release();
#endif
                    }

                    --this->PoolCount;
                    this->Capacity -= currentCapacity;
                    this->FreeCount -= freeCount;

#ifdef OBJA_UNINTERRUPTED
                    System::Interrupts::RestoreState(cookie);
#endif

                    return HandleResult::Okay;
                }
                else
                {
                    //  So, for whatever reason, the removing failed.
                    //  Now this is practically a fresh pool.

#ifdef OBJA_MULTICONSUMER
                    if (previous != nullptr)
                        previous->PropertiesLock.Release();
                    else
                        this->LinkageLock.Release();
                    //  These need not be locked anymore for what they protect
                    //  shall not be modified.
#endif

                    obj_ind_t const capDiff = currentCapacity - current->Capacity;
                    ssize_t const freeDiff = (ssize_t)freeCount - (ssize_t)current->FreeCount;

#ifdef OBJA_MULTICONSUMER
                    current->PropertiesLock.Release();
#endif

                    this->Capacity -= capDiff;
                    this->FreeCount -= freeDiff;
                    //  The sign shouldn't matter, rite? At worst this is -1.

#ifdef OBJA_UNINTERRUPTED
                    System::Interrupts::RestoreState(cookie);
#endif

                    return HandleResult::Okay;
                }

                //  This point cannot be reached.
            }

            //  This one can, if the release options and circumstances forbid
            //  releasing the pool.

#ifdef OBJA_MULTICONSUMER
            //  Vanilla removal will occur now.

            if (canReleaseAny)
            {
                if (previous != nullptr)
                    previous->PropertiesLock.Release();
                else
                    this->LinkageLock.Release();
                //  Release ASAP.
            }
#endif

            if (busyByte != nullptr)
                *busyByte &= ~(1 << (this->BusyBit & 7));
            //  This just clears the busy bit, synchronously.

            FreeObject * const freeObject = (FreeObject *)(uintptr_t)object;
            freeObject->Next = current->FirstFreeObject;

            current->FirstFreeObject = ind;
            ++current->FreeCount;
            //  Book-keeping...

#ifdef OBJA_MULTICONSUMER
            current->PropertiesLock.Release();
            //  Release the current pool and restore interrupts.
#endif

#ifdef OBJA_UNINTERRUPTED
            System::Interrupts::RestoreState(cookie);
#endif

            ++this->FreeCount;
            //  I hate book-keeping.

            return HandleResult::Okay;
        }
        else
        {
            //  If the current one doesn't contain the object, try the next.

#ifdef OBJA_MULTICONSUMER
            if (canReleaseAny)
            {
                if (previous != nullptr)
                    previous->PropertiesLock.Release();
                else
                    this->LinkageLock.Release();
            }

            //  First thing to do is release the previous pool or the chain.
#endif

            OBJA_POOL_TYPE * temp = reinterpret_cast<OBJA_POOL_TYPE *>(current->Next);

            if (temp == nullptr)
            {
                //  THIS IS THE END! (of the chain)

#ifdef OBJA_MULTICONSUMER
                current->PropertiesLock.Release();
                //  Release the current pool and restore interrupts.
#endif

#ifdef OBJA_UNINTERRUPTED
                System::Interrupts::RestoreState(cookie);
#endif

                break;
                //  Go to end of the loop.
            }

            //  Otherwise...

#ifdef OBJA_MULTICONSUMER
            temp->PropertiesLock.Acquire();
            //  Lock the next, keep the current locked.

            if (!canReleaseAny)
                current->PropertiesLock.Release();
            //  If no pool can be released, there's no point in keeping the
            //  previous one locked because its `Next` pointer will never change.
#endif

            previous = current;
            current = temp;
        }
    } while (current != nullptr);

    return HandleResult::ArgumentOutOfRange;
    //  If this point is reached, it means the target object is outside of this allocator's
    //  pools.
}

void OBJA_ALOC_TYPE::Dispose()
{
#ifdef OBJA_UNINTERRUPTED
    OBJA_COOK_TYPE volatile cookie = System::Interrupts::PushDisable();
#endif

    OBJA_POOL_TYPE * current, * next;

#ifdef OBJA_MULTICONSUMER
    this->LinkageLock.Acquire();

    current = this->FirstPool;

    do
    {
        current->PropertiesLock.Acquire();
        
        current = reinterpret_cast<OBJA_POOL_TYPE *>(current->Next);
    } while (current != nullptr);

    //  First thing that needs to be done here is locking all the pools.
    //  This will make sure that they are not being used. As for the objects in
    //  them... Nothing I can do. :(
#endif

    do
    {
        next = reinterpret_cast<OBJA_POOL_TYPE *>(current->Next);

        Handle res = this->ReleasePool(this->ObjectSize, this->HeaderSize, current);
        //  This really shouldn't fail.

        if unlikely(!res.IsOkayResult())
        {
            msg("Failed to release pool %Xp when destructing allocator %Xp..?"
                " (%H)%n"
                , current, this, res);

            //  Moves onto the next one anyway.
        }

        current = next;
    } while (current != nullptr);

    this->FirstPool = nullptr;

    this->AcquirePool = nullptr;
    this->EnlargePool = nullptr;
    this->ReleasePool = nullptr;

    this->PoolCount = 0;
    this->Capacity = 0;
    this->FreeCount = 0;
    this->BusyCount = 0;
    this->Quota = 0;

#ifdef OBJA_MULTICONSUMER
    this->LinkageLock.Release();
#endif

#ifdef OBJA_UNINTERRUPTED
    System::Interrupts::RestoreState(cookie);
#endif
}
