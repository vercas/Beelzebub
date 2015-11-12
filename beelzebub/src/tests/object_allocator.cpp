#ifdef __BEELZEBUB__TEST_OBJA

#include <tests/object_allocator.hpp>
#include <memory/object_allocator.hpp>
#include <memory/page_allocator.hpp>
#include <memory/manager_amd64.hpp>
#include <kernel.hpp>
#include <math.h>
#include <debug.hpp>

#define REPETITION_COUNT   ((size_t)2)
#define REPETITION_COUNT_3 (REPETITION_COUNT * REPETITION_COUNT * REPETITION_COUNT)

#define __BEELZEBUB__TEST_OBJA_ASSERTIONS

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;

SmpBarrier ObjectAllocatorTestBarrier1 {};
SmpBarrier ObjectAllocatorTestBarrier2 {};
SmpBarrier ObjectAllocatorTestBarrier3 {};

struct TestStructure
{
    uint64_t Qwords[3];
    uint32_t Dwords[3];
    uint16_t Words[3];
    uint16_t Bytes[3];
    TestStructure * Next;
};

ObjectAllocator testAllocator;
SpinlockUninterruptible<> syncer;

bool askedToEnlarge, askedToRemove;

__bland Handle AcquirePoolTest(size_t objectSize, size_t headerSize, size_t minimumObjects, ObjectPool * & result)
{
    size_t const pageCount = RoundUp(objectSize * minimumObjects + headerSize, PageSize) / PageSize;

    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    vaddr_t const vaddr = MemoryManagerAmd64::KernelHeapCursor.FetchAdd(pageCount * PageSize);

    for (size_t i = 0; i < pageCount; ++i)
    {
        paddr_t const paddr = Cpu::GetDomain()->PhysicalAllocator->AllocatePage(desc);
        //  Test page.

        assert_or(paddr != nullpaddr && desc != nullptr
            , "Unable to allocate physical page #%us for an object pool (%us, %us, %us, %us)!"
            , i
            , objectSize, headerSize, minimumObjects, pageCount)
        {
            return res;
            //  Maybe the test is built in release mode.
        }

        res = Cpu::GetActiveThread()->Owner->Memory->MapPage(vaddr + i * PageSize, paddr, PageFlags::Global | PageFlags::Writable, desc);

        assert_or(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP; #%us) for an object pool (%us, %us, %us, %us): %H."
            , vaddr + i * PageSize, paddr, i
            , objectSize, headerSize, minimumObjects, pageCount
            , res)
        {
            return res;
            //  Again, maybe the test is built in release mode.
        }
    }

    ObjectPool * pool = result = (ObjectPool *)(uintptr_t)vaddr;
    //  I use a local variable here so `result` isn't dereferenced every time.

    new (pool) ObjectPool();
    //  Construct in place to initialize the fields.

    size_t const objectCount = ((pageCount * PageSize) - headerSize) / objectSize;

    pool->Capacity = pool->FreeCount = objectCount;

    msg("<< Instanced object pool @%Xp with capacity %us (%us pages), "
        "header size %us, object size %us. >>%n"
        , pool, objectCount, pageCount, headerSize, objectSize);

    uintptr_t cursor = (uintptr_t)pool + headerSize;
    FreeObject * last = nullptr;

    for (obj_ind_t i = 0; i < objectCount; ++i, cursor += objectSize)
    {
        //  Note: `cursor` is incremented in the loop construct.
        //  This loops just set the previous object's `Next` pointer to the
        //  index of the current object. If there is no previous object,
        //  the pool's first object is set to the index of the current object.

        FreeObject * const obj = (FreeObject *)cursor;

        if unlikely(last == nullptr)
            pool->FirstFreeObject = i;
        else
            last->Next = i;

        last = obj;
    }

    //  After the loop is finished, `last` will point to the very last object
    //  in the pool. `objectCount - 1` will be the index of the last object.

    pool->LastFreeObject = (obj_ind_t)objectCount - 1;
    last->Next = obj_ind_invalid;

    //  The pool was constructed in place, so the rest of the fields should
    //  be in a good state.

    return HandleResult::Okay;
}

__bland Handle EnlargePoolTest(size_t objectSize, size_t headerSize, size_t minimumExtraObjects, ObjectPool * pool)
{
    askedToEnlarge = true;

    /*msg("~~ ASKED TO ENLARGE POOL %Xp ~~%n"
        , pool);//*/

    size_t const oldPageCount = RoundUp(objectSize * pool->Capacity + headerSize, PageSize) / PageSize;
    size_t newPageCount = RoundUp(objectSize * (pool->Capacity + minimumExtraObjects) + headerSize, PageSize) / PageSize;

    ASSERT(newPageCount > oldPageCount
        , "New page count (%us) should be larger than the old page count (%us) "
          "of a pool that needs enlarging!%nIt appears that the previous capacity"
          "is wrong.%n"
        , newPageCount, oldPageCount);

    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    vaddr_t const vaddr = oldPageCount * PageSize + (vaddr_t)pool;

    vaddr_t oldPoolEnd = vaddr;
    vaddr_t newPoolEnd = vaddr + (newPageCount - oldPageCount) * PageSize;

    /*msg("~~ FROM %Xp (%us pages) to %Xp (%us pages) ~~%n"
        , oldPoolEnd, oldPageCount, newPoolEnd, newPageCount);//*/

    if unlikely(newPoolEnd > MemoryManagerAmd64::KernelHeapEnd)
    {
        newPageCount -= (newPoolEnd - MemoryManagerAmd64::KernelHeapEnd) / PageSize;
        newPoolEnd = vaddr + (newPageCount - oldPageCount) * PageSize;

        /*msg("~~ REDUCED END TO %Xp (%us pages) ~~%n", newPoolEnd, newPageCount);//*/
    }

    bool const swapped = MemoryManagerAmd64::KernelHeapCursor.CmpXchgStrong(oldPoolEnd, newPoolEnd);

    if (!swapped)
        return HandleResult::PageMapped;
    //  It is possible that something else has already reserved memory here.
    //  TODO: Check for actual free pages.

    vaddr_t curPageCount = oldPageCount;

    for (size_t i = 0; curPageCount < newPageCount; ++i, ++curPageCount)
    {
        paddr_t const paddr = Cpu::GetDomain()->PhysicalAllocator->AllocatePage(desc);
        //  Test page.

        assert_or(paddr != nullpaddr && desc != nullptr
            , "Unable to allocate physical page #%us for extending object pool "
              "%Xp (%us, %us, %us, %us, %us)!"
            , i, pool
            , objectSize, headerSize, minimumExtraObjects, oldPageCount, newPageCount)
        {
            break;
        }

        res = Cpu::GetActiveThread()->Owner->Memory->MapPage(vaddr + i * PageSize, paddr, PageFlags::Global | PageFlags::Writable, desc);

        assert_or(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP; #%us) for extending object pool "
              "%Xp (%us, %us, %us, %us, %us): %H."
            , vaddr + i * PageSize, paddr, i, pool
            , objectSize, headerSize, minimumExtraObjects, oldPageCount, newPageCount
            , res)
        {
            Cpu::GetDomain()->PhysicalAllocator->FreePageAtAddress(paddr);
            //  This should succeed.

            break;
        }
    }

    if (curPageCount == oldPageCount)
        return res;
    //  Nothing was allocated.

    /*msg("~~ NEW PAGE COUNT IS %us ~~%n", curPageCount);//*/

    obj_ind_t const oldObjectCount = pool->Capacity;
    obj_ind_t const newObjectCount = ((curPageCount * PageSize) - headerSize) / objectSize;

    /*msg("~~ OBJECTS INCREASED FROM %u4 to %u4 ~~%n", oldObjectCount, newObjectCount);//*/

    uintptr_t cursor = (uintptr_t)pool + headerSize + oldObjectCount * objectSize;
    FreeObject * last = nullptr;

    if (pool->FreeCount > 0)
        last = pool->GetLastFreeObject(objectSize, headerSize);

    /*msg("~~ LAST FREE OBJECT @ %Xp (%u4 free objects) ~~%n"
        , last, pool->FreeCount);//*/

    for (obj_ind_t i = oldObjectCount; i < newObjectCount; ++i, cursor += objectSize)
    {
        FreeObject * const obj = (FreeObject *)cursor;

        if unlikely(last == nullptr)
            pool->FirstFreeObject = i;
        else
            last->Next = i;

        last = obj;
    }

    //  After the loop is finished, `last` will point to the very last object
    //  in the pool. `newObjectCount - 1` will be the index of the last object.

    pool->LastFreeObject = (obj_ind_t)newObjectCount - 1;
    last->Next = obj_ind_invalid;

    pool->Capacity = newObjectCount;
    pool->FreeCount += newObjectCount - oldObjectCount;
    //  This is incremented because they could've been free objects prior to
    //  enlarging the pool.

    return HandleResult::Okay;
}

__bland Handle ReleasePoolTest(size_t objectSize, size_t headerSize, ObjectPool * pool)
{
    askedToRemove = true;

    /*msg("~~ ASKED TO REMOVE POOL %Xp ~~%n"
        , pool);//*/

    //  A nice procedure here is to unmap the pool's pages one-by-one, starting
    //  from the highest. The kernel heap cursor will be pulled back if possible.

    Handle res;

    bool decrementedHeapCursor = true;
    //  Initial value is for simplifying the algorithm below.

    size_t const pageCount = RoundUp(objectSize * pool->Capacity + headerSize, PageSize) / PageSize;

    vaddr_t vaddr = (vaddr_t)pool + (pageCount - 1) * PageSize;
    size_t i = pageCount;
    for (/* nothing */; i > 0; --i, vaddr -= PageSize)
    {
        /*msg("~~ UNMAPPING PAGE %Xp FROM POOL %Xp ~~%n", vaddr, pool);//*/

        res = Cpu::GetActiveThread()->Owner->Memory->UnmapPage(vaddr);

        assert_or(res.IsOkayResult()
            , "Failed to unmap page #%us from pool %Xp.%n"
            , i, pool)
        {
            break;

            //  The rest of the function will attempt to adapt.
        }

        vaddr_t expectedCursor = vaddr + PageSize;

        if (decrementedHeapCursor)
            decrementedHeapCursor = MemoryManagerAmd64::KernelHeapCursor.CmpXchgStrong(expectedCursor, vaddr);
    }

    //  TODO: Salvage pool when it failed to remove all pages.
    //if (i > 0 && i < (pageCount - 1))

    return i < (pageCount - 1)
        ? HandleResult::Okay // At least one page was freed so the pool needs removal...
        : HandleResult::UnsupportedOperation;
}

/**************************
    Actual Testing Code
**************************/

#define TESTALLOC(T, n)                                                    \
    T * MCATS(tO, n) = nullptr;                                            \
    res = testAllocator.AllocateObject(MCATS(tO, n));                      \
    msg_("Attempted to allocate an object: %H; %Xp.%n", res, MCATS(tO, n));

#define TESTREMOV(n)                                                       \
    res = testAllocator.DeallocateObject(MCATS(tO, n));                    \
    msg_("Attempted to delete an object: %H; %Xp.%n", res, MCATS(tO, n));

#define TESTALLOC2(T, n)                                                   \
    T * MCATS(tO, n) = nullptr;                                            \
    res = testAllocator.AllocateObject(MCATS(tO, n));

#define TESTREMOV2(n)                                                      \
    res = testAllocator.DeallocateObject(MCATS(tO, n));

#define TESTALLOC3(T, n)                                                   \
    T * MCATS(tO, n) = nullptr;                                            \
    res = testAllocator.AllocateObject(MCATS(tO, n));                      \
    ASSERT(res.IsOkayResult()                                              \
        , "Failed to allocate object \"" #n "\": %H%n"                     \
        , res);

#define TESTREMOV3(n)                                                      \
    res = testAllocator.DeallocateObject(MCATS(tO, n));                    \
    ASSERT(res.IsOkayResult()                                              \
        , "Failed to delete object \"" #n "\" (%Xp): %H%n"                 \
        , MCATS(tO, n), res);

#define TESTDIFF(n1, n2) ASSERT(MCATS(tO, n1) != MCATS(tO, n2)             \
    , "Test objects \"" #n1 "\" and \"" #n2 "\" should be different: %Xp." \
    , MCATS(tO, n1));

__bland Handle CommonObjectAllocatorTest()
{
    Handle res;

    ObjectAllocatorTestBarrier1.Reach();

    size_t volatile freeCount1 = testAllocator.GetFreeCount();
    size_t volatile capacity1 = testAllocator.GetCapacity();

    ObjectAllocatorTestBarrier2.Reach();

    ObjectAllocatorTestBarrier1.Reset(); //  Prepare the first barrier for re-use.

    uint64_t perfAcc = 0;

    for (size_t x = REPETITION_COUNT; x > 0; --x)
    {
        uint64_t perfStart = CpuInstructions::Rdtsc();

        TESTALLOC2(TestStructure, A)

#ifdef __BEELZEBUB__TEST_OBJA_ASSERTIONS
        ASSERT(res.IsOkayResult()
            , "Failed to allocate test object A for repetition %us: %H"
            , 1 + REPETITION_COUNT - x, res);
#endif        

        for (size_t y = REPETITION_COUNT; y > 0; --y)
        {
            TESTALLOC2(TestStructure, B)

#ifdef __BEELZEBUB__TEST_OBJA_ASSERTIONS
            ASSERT(res.IsOkayResult()
                , "Failed to allocate test object B for repetition %us->%us: %H"
                , 1 + REPETITION_COUNT - x, 1 + REPETITION_COUNT - y, res);
#endif            

            TESTDIFF(A, B)

            for (size_t z = REPETITION_COUNT; z > 0; --z)
            {
                TESTALLOC2(TestStructure, C)

#ifdef __BEELZEBUB__TEST_OBJA_ASSERTIONS
                ASSERT(res.IsOkayResult()
                    , "Failed to allocate test object C for repetition %us->%us->%us: %H"
                    , 1 + REPETITION_COUNT - x, 1 + REPETITION_COUNT - y, 1 + REPETITION_COUNT - z, res);
#endif                

                TESTDIFF(A, C)
                TESTDIFF(B, C)

                TESTREMOV2(C)

#ifdef __BEELZEBUB__TEST_OBJA_ASSERTIONS
                ASSERT(res.IsOkayResult()
                    , "Failed to remove test object C (%Xp) for repetition %us->%us->%us: %H"
                    , tOC, 1 + REPETITION_COUNT - x, 1 + REPETITION_COUNT - y, 1 + REPETITION_COUNT - z, res);
#endif                
            }

            TESTREMOV2(B)

#ifdef __BEELZEBUB__TEST_OBJA_ASSERTIONS
            ASSERT(res.IsOkayResult()
                , "Failed to remove test object B (%Xp) for repetition %us->%us: %H"
                , tOB, 1 + REPETITION_COUNT - x, 1 + REPETITION_COUNT - y, res);
#endif            
        }

        TESTREMOV2(A)

#ifdef __BEELZEBUB__TEST_OBJA_ASSERTIONS
        ASSERT(res.IsOkayResult()
            , "Failed to remove test object A (%Xp) for repetition %us: %H"
            , tOA, 1 + REPETITION_COUNT - x, res);
#endif        

        uint64_t perfEnd = CpuInstructions::Rdtsc();

        perfAcc += perfEnd - perfStart;
    }

    MSG_("Core %us: %u8 / %us = %u8; %u8 / %us = %u8; %n"
        , Cpu::GetIndex()
        , perfAcc, REPETITION_COUNT, perfAcc / REPETITION_COUNT
        , perfAcc, REPETITION_COUNT_3, perfAcc / REPETITION_COUNT_3);

    ObjectAllocatorTestBarrier3.Reach();

    ObjectAllocatorTestBarrier2.Reset(); //  Prepare the second barrier for re-use.

    ASSERT(capacity1 - freeCount1 == testAllocator.GetCapacity() - testAllocator.GetFreeCount()
        , "Allocator's busy object count has a shady value: %us (%us - %us)"
          ", expected %us (%us - %us)."
        , testAllocator.GetCapacity() - testAllocator.GetFreeCount()
        , testAllocator.GetCapacity(), testAllocator.GetFreeCount()
        , capacity1 - freeCount1, capacity1, freeCount1);

    ObjectAllocatorTestBarrier1.Reach();

    ObjectAllocatorTestBarrier3.Reset(); //  Prepare the third barrier for re-use.

    return HandleResult::Okay;
}

Handle TestObjectAllocator(bool const bsp)
{
    Handle res;

    if likely(!bsp)
        return CommonObjectAllocatorTest();
    else
    {
        new (&testAllocator) ObjectAllocator(sizeof(TestStructure), __alignof(TestStructure), &AcquirePoolTest, &EnlargePoolTest, &ReleasePoolTest, true);

        msg("Test allocator (%Xp): Capacity = %Xs, Free Count = %Xs, Pool Count = %Xs;%n"
            , &testAllocator
            , testAllocator.Capacity.Load()
            , testAllocator.FreeCount.Load()
            , testAllocator.PoolCount.Load());

        askedToRemove = false;

        TESTALLOC3(TestStructure, 1)
        TESTALLOC3(TestStructure, 2)
        TESTALLOC3(TestStructure, 3)
        TESTALLOC3(TestStructure, 4)

        TESTREMOV3(2)

        TESTALLOC3(TestStructure, 5)

        ASSERT(tO2 == tO5
            , "2nd and 5th test objects should be identical: %Xp vs %Xp"
            , tO2, tO5);

        TESTDIFF(1, 2)
        TESTDIFF(1, 3)
        TESTDIFF(1, 4)
        TESTDIFF(2, 3)
        TESTDIFF(2, 4)
        TESTDIFF(3, 4)

        msg("Test allocator (%Xp): Capacity = %us, Free Count = %us, Pool Count = %us;%n"
            , &testAllocator
            , testAllocator.Capacity.Load()
            , testAllocator.FreeCount.Load()
            , testAllocator.PoolCount.Load());

        ASSERT(testAllocator.PoolCount == 1
            , "Test allocator should have only one pool, not %us.%n"
            , testAllocator.PoolCount.Load());

        ASSERT(testAllocator.GetCapacity() - testAllocator.GetFreeCount() == 4
            , "Test allocator should have only 4 objects non-free, not %us "
              "(%us - %us).%n"
            , testAllocator.GetCapacity() - testAllocator.GetFreeCount()
            , testAllocator.GetCapacity(), testAllocator.GetFreeCount());

        res = CommonObjectAllocatorTest();

        if (!res.IsOkayResult())
            return res;

        askedToEnlarge = false;

        TestStructure * tOx = nullptr, * tOy = nullptr;

        for (size_t i = testAllocator.GetFreeCount(); i > 0; --i)
        {
            tOy = tOx;

            if unlikely(i == 1)
            {
                ASSERT(testAllocator.GetFreeCount() == 1
                    , "Test allocator should only have one free object, not %us.%n"
                    , testAllocator.GetFreeCount());

                ASSERT(!askedToEnlarge
                    , "The allocator asked to enlarge a pool before it was full..?");
            }

            res = testAllocator.AllocateObject(tOx);

            ASSERT(res.IsOkayResult()
                , "Failed to allocate capacity-filling object #%us: %H%n"
                , i, res);

            TESTDIFF(x, y);

            if (tOy != nullptr)
                ASSERT(tOy->Next != tOx
                    , "Previous test object points to the current one..?");

            tOx->Next = tOy;
        }

        ASSERT(askedToEnlarge
            , "The allocator should have asked to enlarge a pool after "
              "allocating the last object!");

        //  Now let's allocate moar objects!

        TESTALLOC3(TestStructure, 11)
        TESTALLOC3(TestStructure, 12)
        TESTALLOC3(TestStructure, 13)
        TESTALLOC3(TestStructure, 14)

        TESTREMOV3(13)

        TESTALLOC3(TestStructure, 15)

        ASSERT(tO13 == tO15
            , "13th and 15th test objects should be identical: %Xp vs %Xp"
            , tO13, tO15);

        TESTDIFF(11, 12) TESTDIFF(11, 1) TESTDIFF(11, 2) TESTDIFF(11, x) TESTDIFF(11, y)
        TESTDIFF(11, 13) TESTDIFF(11, 1) TESTDIFF(11, 3) TESTDIFF(11, x) TESTDIFF(11, y)
        TESTDIFF(11, 14) TESTDIFF(11, 1) TESTDIFF(11, 4) TESTDIFF(11, x) TESTDIFF(11, y)
        TESTDIFF(12, 13) TESTDIFF(12, 2) TESTDIFF(12, 3) TESTDIFF(12, x) TESTDIFF(12, y)
        TESTDIFF(12, 14) TESTDIFF(12, 2) TESTDIFF(12, 4) TESTDIFF(12, x) TESTDIFF(12, y)
        TESTDIFF(13, 14) TESTDIFF(13, 3) TESTDIFF(13, 4) TESTDIFF(13, x) TESTDIFF(13, y)

        ASSERT(!askedToRemove
            , "The allocator asked to remove a pool before it was empty..?");

        //  Let's try full removal.

        TESTREMOV3( 3) TESTREMOV3( 1) TESTREMOV3( 4) TESTREMOV3( 5)
        TESTREMOV3(12) TESTREMOV3(11) TESTREMOV3(14) TESTREMOV3(15)
        //  Removing known objects.

        while (tOy != nullptr)
        {
            auto next = tOy->Next;

            TESTREMOV3(y)
            //  Simple, huh?

            tOy = next;
        }

        //  Right now there should only be one object left in the pool: tOx.

        ASSERT(!askedToRemove
            , "The allocator asked to remove a pool when one object should be left in it!");

        TESTREMOV3(x)

        ASSERT(askedToRemove
            , "The allocator should have asked to remove a pool after "
              "deallocating the last object!");

        return HandleResult::Okay;
    }
}

#endif
