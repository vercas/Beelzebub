#ifdef __BEELZEBUB__TEST_OBJA

#include <tests/object_allocator.hpp>
#include <memory/object_allocator.hpp>
#include <memory/page_allocator.hpp>
#include <memory/manager_amd64.hpp>
#include <kernel.hpp>
#include <math.h>
#include <debug.hpp>

#define REPETITION_COUNT   ((size_t)200)
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
};

ObjectAllocator testAllocator;
SpinlockUninterruptible<> syncer;

__bland Handle AcquirePoolTest(size_t objectSize, size_t headerSize, size_t minimumObjects, ObjectPool * & result)
{
    size_t const pageCount = RoundUp(objectSize + minimumObjects * headerSize, PageSize);

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

    msg("<< Instanced object pool @%Xp with capacity %us, "
        "header size %us, object size %us. >>%n"
        , pool, objectCount, headerSize, objectSize);

    uintptr_t cursor = (uintptr_t)pool + headerSize;
    FreeObject * last = nullptr;

    for (obj_ind_t i = 0; i < objectCount; ++i, cursor += objectSize)
    {
        //  Note: `cursor` is incremented in the loop construct.
        //  This loops just set the previous object's `Next` pointer to the
        //  index of the current object. If there is no previous object,
        //  the pool's first object is set to the index of the current object.

        FreeObject * obj = (FreeObject *)cursor;

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
    msg("~~ ASKED TO ENLARGE POOL %Xp ~~%n"
        , pool);

    return HandleResult::UnsupportedOperation;
}

__bland Handle ReleasePoolTest(size_t objectSize, size_t headerSize, ObjectPool * pool)
{

}

Handle TestObjectAllocator(bool const bsp)
{
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

    Handle res;

    if (bsp)
    {
        new (&testAllocator) ObjectAllocator(sizeof(TestStructure), __alignof(TestStructure), &AcquirePoolTest, &EnlargePoolTest, &ReleasePoolTest, true);

        msg("Test allocator (%Xp): Capacity = %Xs, Free Count = %Xs, Pool Count = %Xs;%n"
            , &testAllocator
            , testAllocator.Capacity.Load()
            , testAllocator.FreeCount.Load()
            , testAllocator.PoolCount.Load());

        TESTALLOC3(TestStructure, 1)
        TESTALLOC3(TestStructure, 2)
        TESTALLOC3(TestStructure, 3)
        TESTALLOC3(TestStructure, 4)

        TESTREMOV3(2);

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
    }

    ObjectAllocatorTestBarrier1.Reach();

#ifdef __BEELZEBUB__TEST_OBJA_ASSERTIONS
    size_t volatile freeCount1 = testAllocator.GetFreeCount();
#endif

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

#ifdef __BEELZEBUB__TEST_OBJA_ASSERTIONS
    ASSERT(freeCount1 == testAllocator.GetFreeCount()
        , "Allocator's free count has a shady value: %us, expected %us."
        , testAllocator.GetFreeCount(), freeCount1);
#endif    

    return HandleResult::Okay;
}

#endif
