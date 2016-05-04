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

#ifdef OBJA_POOL_TYPE
/**
 *  <summary>Represents an area of memory where objects can be allocated.</summary>
 */
struct OBJA_POOL_TYPE : public ObjectPoolBase
{
    /*  Fields  */

    OBJA_LOCK_TYPE PropertiesLock;

    /*  Constructors  */

    inline OBJA_POOL_TYPE()
        : ObjectPoolBase()
        , PropertiesLock()
    {

    }

    OBJA_POOL_TYPE(OBJA_POOL_TYPE const &) = delete;
    OBJA_POOL_TYPE & operator =(const OBJA_POOL_TYPE &) = delete;
    //  Aye - no copying here either.
};
#else
    #define OBJA_POOL_TYPE ObjectPoolBase
#endif

/**
 *  <summary>Manages pools of fixed-size objects.</summary>
 */
class OBJA_ALOC_TYPE
{
public:

    /*  Constructors  */

    inline OBJA_ALOC_TYPE()
        : AcquirePool( nullptr)
        , EnlargePool(nullptr)
        , ReleasePool(nullptr)
        , ObjectSize(0)
        , HeaderSize(0)
        , FirstPool(nullptr)
#ifdef OBJA_MULTICONSUMER
        , LinkageLock()
#endif
        , Capacity(0)
        , FreeCount(0)
        , PoolCount(0)
        , ReleaseOptions(PoolReleaseOptions::ReleaseAll)
        , BusyBit(SIZE_MAX)
        , BusyCount(0)
        , Quota(0)    //  This allocator cannot even be used!
    {
        //  This constructor is required because of the const fields.
    }
         
    OBJA_ALOC_TYPE(OBJA_ALOC_TYPE const &) = delete;
    OBJA_ALOC_TYPE & operator =(const OBJA_ALOC_TYPE &) = delete;

    OBJA_ALOC_TYPE(size_t const objectSize, size_t const objectAlignment
        , AcquirePoolFunc acquirer, EnlargePoolFunc enlarger, ReleasePoolFunc releaser
        , PoolReleaseOptions const releaseOptions = PoolReleaseOptions::ReleaseAll
        , size_t const busyBit = SIZE_MAX, size_t const quota = SIZE_MAX);

    /*  Methods  */

    template<typename T>
    inline Handle AllocateObject(T * & result, size_t estimatedLeft = 1)
    {
        if (sizeof(T) > this->ObjectSize)
            return HandleResult::ArgumentTemplateInvalid;
        
        //  Yes, smaller objects will be accepted.

        void * pRes;

        Handle hRes = this->AllocateObject(pRes, estimatedLeft);

        result = (T *)pRes;

        return hRes;
    }

    __hot __noinline Handle AllocateObject(void * & result, size_t estimatedLeft = 1);
    __hot __noinline Handle DeallocateObject(void * const object);
    //  These are complex methods and GCC will not be intimidated.

    /// <summary>Performs total and utter destruction of the allocator.</summary>
    __cold __noinline void Dispose();

    /*  Properties  */

#ifdef OBJA_MULTICONSUMER
    inline size_t GetCapacity()     const { return this->Capacity.Load();   }
    inline size_t GetFreeCount()    const { return this->FreeCount.Load();  }
    inline size_t GetQuota()        const { return this->Quota.Load();      }
    inline size_t GetBusyCount()    const { return this->BusyCount.Load();  }
#else
    inline size_t GetCapacity()     const { return this->Capacity;          }
    inline size_t GetFreeCount()    const { return this->FreeCount;         }
    inline size_t GetQuota()        const { return this->Quota;             }
    inline size_t GetBusyCount()    const { return this->BusyCount;         }
#endif

    inline size_t GetClearance() const
    {
        size_t const busyCount = this->GetBusyCount();
        size_t const max = this->GetQuota();

        if (busyCount <= max)
            return max - busyCount;
        else
            return 0;

        //  The case when BusyCount > Quota is handled because the maximum
        //  may be changed at any time.
    }

    /*  Parameters  */

private:

    AcquirePoolFunc AcquirePool;    //  When this is null, the allocator is
    EnlargePoolFunc EnlargePool;    //  destructed or invalid.
    ReleasePoolFunc ReleasePool;

public:

    size_t const ObjectSize;
    size_t const HeaderSize;

private:

    /*  Links  */

    OBJA_POOL_TYPE * volatile FirstPool;

#ifdef OBJA_MULTICONSUMER
    OBJA_LOCK_TYPE LinkageLock;
#endif

    /*  Stats  */

#ifdef OBJA_MULTICONSUMER
    Beelzebub::Synchronization::Atomic<size_t> Capacity;
    Beelzebub::Synchronization::Atomic<size_t> FreeCount;
    Beelzebub::Synchronization::Atomic<size_t> PoolCount;
#else
    size_t Capacity;
    size_t FreeCount;
    size_t PoolCount;
#endif

    /*  Config  */

    PoolReleaseOptions const ReleaseOptions;
    size_t const BusyBit;

#ifdef OBJA_MULTICONSUMER
    Beelzebub::Synchronization::Atomic<size_t> BusyCount;
#else
    size_t BusyCount;
#endif

public:

    //  Yes, this is public and non-const.
#ifdef OBJA_MULTICONSUMER
    Beelzebub::Synchronization::Atomic<size_t> Quota;
#else
    size_t Quota;
#endif
};
