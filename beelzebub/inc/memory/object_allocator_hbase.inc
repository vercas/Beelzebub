/**
 *  <summary>Represents an area of memory where objects can be allocated.</summary>
 */
struct OBJA_POOL_TYPE : public ObjectPoolBase
{
    /*  Fields  */

    OBJA_LOCK_TYPE PropertiesLock;

    /*  Constructors  */

    __bland inline OBJA_POOL_TYPE()
        : ObjectPoolBase()
        , PropertiesLock()
    {

    }

    OBJA_POOL_TYPE(OBJA_POOL_TYPE const &) = delete;
    OBJA_POOL_TYPE & operator =(const OBJA_POOL_TYPE &) = delete;
    //  Aye - no copying here either.
};

/**
 *  <summary>Manages pools of fixed-size objects.</summary>
 */
class OBJA_ALOC_TYPE
{
public:

    /*  Constructors  */

    __bland inline OBJA_ALOC_TYPE()
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
        , CanReleaseAllPools(true)
    {
        //  This constructor is required because of the const fields.
    }
         
    OBJA_ALOC_TYPE(OBJA_ALOC_TYPE const &) = delete;
    OBJA_ALOC_TYPE & operator =(const OBJA_ALOC_TYPE &) = delete;

    __bland OBJA_ALOC_TYPE(size_t const objectSize, size_t const objectAlignment, AcquirePoolFunc acquirer, EnlargePoolFunc enlarger, ReleasePoolFunc releaser, bool const canReleaseAll = false);

    /*  Methods  */

    template<typename T>
    __bland inline Handle AllocateObject(T * & result, size_t estimatedLeft = 1)
    {
        if (sizeof(T) > this->ObjectSize)
            return HandleResult::ArgumentTemplateInvalid;
        
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

#ifdef OBJA_MULTICONSUMER
    __bland inline size_t GetCapacity() const
    {
        return this->Capacity.Load();
    }

    __bland inline size_t GetFreeCount() const
    {
        return this->FreeCount.Load();
    }
#else
    __bland inline size_t GetCapacity() const
    {
        return this->Capacity;
    }

    __bland inline size_t GetFreeCount() const
    {
        return this->FreeCount;
    }
#endif

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

    OBJA_POOL_TYPE * volatile FirstPool;

#ifdef OBJA_MULTICONSUMER
    OBJA_LOCK_TYPE LinkageLock;
#endif

    /*  Stats  */

#ifdef OBJA_MULTICONSUMER
    Atomic<size_t> Capacity;
    Atomic<size_t> FreeCount;
    Atomic<size_t> PoolCount;
#else
    size_t Capacity;
    size_t FreeCount;
    size_t PoolCount;
#endif

    /*  Config  */

    bool const CanReleaseAllPools;
};
