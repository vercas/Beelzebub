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

ObjectAllocator::ObjectAllocator(const size_t objectSize, const size_t objectAlignment, AcquirePoolFunc acquirer, EnlargePoolFunc enlarger, ReleasePoolFunc releaser)
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
{
    //  As you can see, at least a FreeObject must fit in the object size.
    //  Also, only the alignment of the 
}

/*  Methods  */

Handle ObjectAllocator::AllocateObject(void * & result, size_t estimatedLeft)
{
    Handle res;
    ObjectAllocatorLockCookie cookie;

    ObjectPool * first = nullptr, * current = nullptr, * justAllocated = nullptr;

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
            current->FreeCount--;

            FreeObject * obj = current->GetFirstFreeObject(this->ObjectSize, this->HeaderSize);
            current->FirstFreeObject = obj->Next;
        }
    } while (current != first);
    //  Yes, the condition here is redundant. But it exists just so the compiler doesn't think this loops forever.
    //  Not implying that the compiler would think that, but I've met my fair share of compiler bugs which lead
    //  to me building up this paranoia.

    return res;
}

Handle ObjectAllocator::DeallocateObject(void * object)
{
    Handle res;

    ObjectPool * first = nullptr, * current = nullptr;

    if unlikely(this->FirstPool == nullptr)
    {
        return HandleResult::ArgumentOutOfRange;
        //  Aye. If there are no allocators, the object does not belong fo fhis
        //  allocator.
    }

    first = current = this->FirstPool;

    do
    {
        if (current->Contains((uintptr_t)object, this->ObjectSize, this->HeaderSize))
        {

        }
    } while (current != first);
}
