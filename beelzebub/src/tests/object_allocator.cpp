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

#ifdef __BEELZEBUB__TEST_OBJA

#include <tests/object_allocator.hpp>
#include <memory/object_allocator_smp.hpp>
#include <memory/page_allocator.hpp>
#include <memory/vmm.hpp>
#include <kernel.hpp>

#include <system/cpu.hpp>
#include <math.h>
#include <debug.hpp>

#define REPETITION_COUNT   ((size_t)20)
#define REPETITION_COUNT_2 (REPETITION_COUNT   * REPETITION_COUNT + REPETITION_COUNT)
#define REPETITION_COUNT_3 (REPETITION_COUNT_2 * REPETITION_COUNT + REPETITION_COUNT)

#define __BEELZEBUB__TEST_OBJA_ASSERTIONS

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

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

static ObjectAllocatorSmp testAllocator;
SpinlockUninterruptible<> syncer;

bool askedToAcquire, askedToEnlarge, askedToRemove, canEnlarge;

static __noinline Handle GetKernelHeapPages(size_t const pageCount, uintptr_t & address)
{
    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    vaddr_t const vaddr = Vmm::KernelHeapCursor.FetchAdd(pageCount * PageSize);

    for (size_t i = 0; i < pageCount; ++i)
    {
        paddr_t const paddr = Cpu::GetData()->DomainDescriptor->PhysicalAllocator->AllocatePage(desc);
        //  Test page.

        assert_or(paddr != nullpaddr && desc != nullptr
            , "Unable to allocate physical page #%us for an object pool!"
            , i)
        {
            return res;
            //  Maybe the test is built in release mode.
        }

        res = Vmm::MapPage(&BootstrapProcess, vaddr + i * PageSize, paddr
            , MemoryFlags::Global | MemoryFlags::Writable, desc);

        assert_or(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP; #%us) for an object pool (%us pages): %H."
            , vaddr + i * PageSize, paddr, i, pageCount, res)
        {
            return res;
            //  Again, maybe the test is built in release mode.
        }
    }

    address = vaddr;

    return res;
}

static __noinline void FillPool(ObjectPoolBase volatile * volatile pool
                              , size_t const objectSize
                              , size_t const headerSize
                              , obj_ind_t const objectCount)
{
    pool->Capacity = objectCount;
    pool->FreeCount = objectCount;

    /*msg("<< Instanced object pool @%Xp with capacity %u4 (%us), "
        "free count %u4, header size %us, object size %us. >>%n"
        , pool, pool->Capacity, objectCount, pool->FreeCount
        , headerSize, objectSize);//*/
    COMPILER_MEMORY_BARRIER();

    uintptr_t cursor = (uintptr_t)pool + headerSize;
    FreeObject * last = nullptr;

    /*msg("<< cursor=%Xp, cap=%u4, FC=%u4, FFO=%u4 >>%n"
        , cursor, pool->Capacity, pool->FreeCount, pool->FirstFreeObject);//*/

    for (obj_ind_t i = 0; i < objectCount; ++i, cursor += objectSize)
    {
        //  Note: `cursor` is incremented in the loop construct.
        //  This loops just set the previous object's `Next` pointer to the
        //  index of the current object. If there is no previous object,
        //  the pool's first object is set to the index of the current object.

        FreeObject * const obj = (FreeObject *)cursor;

        /*msg("<< FO @ %Xp; ", obj);//*/

        if unlikely(last == nullptr)
        {
            /*msg("BEFORE cap=%u4, FC=%u4, FFO=%u4; "
                , pool->Capacity, pool->FreeCount, pool->FirstFreeObject);//*/

            pool->FirstFreeObject = i;

            /*msg("AFTER cap=%u4, FC=%u4, FFO=%u4 >>%n"
                , pool->Capacity, pool->FreeCount, pool->FirstFreeObject);//*/
        }
        else
        {
            /*msg("BEFORE cap=%u4, FC=%u4, FFO=%u4; "
                , pool->Capacity, pool->FreeCount, pool->FirstFreeObject);//*/

            last->Next = i;

            /*msg("AFTER cap=%u4, FC=%u4, FFO=%u4 >>%n"
                , pool->Capacity, pool->FreeCount, pool->FirstFreeObject);//*/
        }

        last = obj;
    }

    //  After the loop is finished, `last` will point to the very last object
    //  in the pool. `pool->Capacity - 1` will be the index of the last object.

    pool->LastFreeObject = objectCount - 1;
    last->Next = obj_ind_invalid;

    /*msg("<< FFO=%u4, LFO=%u4, cursor=%Xp >>%n"
        , pool->FirstFreeObject, pool->LastFreeObject, cursor);
    msg("<< Instanced object pool @%Xp with capacity %us (%u4), "
        "free count %u4. >>%n"
        , pool, pool->Capacity, objectCount, pool->FreeCount);//*/
    COMPILER_MEMORY_BARRIER();
}

Handle AcquirePoolTest(size_t objectSize, size_t headerSize, size_t minimumObjects, ObjectPoolBase * & result)
{
    askedToAcquire = true;

    assert(headerSize >= sizeof(ObjectPoolBase)
        , "The given header size (%us) apprats to be lower than the size of an "
          "actual pool struct (%us)..?%n"
        , headerSize, sizeof(ObjectPoolBase));

    size_t const pageCount = RoundUp(objectSize * minimumObjects + headerSize, PageSize) / PageSize;
    uintptr_t addr = 0;

    Handle res = GetKernelHeapPages(pageCount, addr);

    if (!res.IsOkayResult())
        return res;

    ObjectPoolBase volatile * volatile pool = (ObjectPoolBase *)(uintptr_t)addr;
    //  I use a local variable here so `result` isn't dereferenced every time.

    new (const_cast<ObjectPoolBase *>(pool)) ObjectPoolBase();
    //  Construct in place to initialize the fields.

    size_t const objectCount = ((pageCount * PageSize) - headerSize) / objectSize;
    //  TODO: Get rid of this division and make the loop below stop when the
    //  cursor reaches the end of the page(s).

    FillPool(pool, objectSize, headerSize, (obj_ind_t)objectCount);

    //  The pool was constructed in place, so the rest of the fields should
    //  be in a good state.

    result = const_cast<ObjectPoolBase *>(pool);

    return HandleResult::Okay;
}

Handle EnlargePoolTest(size_t objectSize, size_t headerSize, size_t minimumExtraObjects, ObjectPoolBase * pool)
{
    if (!canEnlarge)
        return HandleResult::UnsupportedOperation;

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

    // if unlikely(newPoolEnd > MemoryManagerAmd64::KernelHeapEnd)
    // {
    //     newPageCount -= (newPoolEnd - MemoryManagerAmd64::KernelHeapEnd) / PageSize;
    //     newPoolEnd = vaddr + (newPageCount - oldPageCount) * PageSize;

    //     /*msg("~~ REDUCED END TO %Xp (%us pages) ~~%n", newPoolEnd, newPageCount);//*/
    // }

    bool const swapped = Vmm::KernelHeapCursor.CmpXchgStrong(oldPoolEnd, newPoolEnd);

    ASSERT(swapped
        , "Failed to compare-exchange the kernel heap cursor!");;

    vaddr_t curPageCount = oldPageCount;

    for (size_t i = 0; curPageCount < newPageCount; ++i, ++curPageCount)
    {
        paddr_t const paddr = Cpu::GetData()->DomainDescriptor->PhysicalAllocator->AllocatePage(desc);
        //  Test page.

        assert_or(paddr != nullpaddr && desc != nullptr
            , "Unable to allocate physical page #%us for extending object pool "
              "%Xp (%us, %us, %us, %us, %us)!"
            , i, pool
            , objectSize, headerSize, minimumExtraObjects, oldPageCount, newPageCount)
        {
            break;
        }

        res = Vmm::MapPage(&BootstrapProcess, vaddr + i * PageSize, paddr
            , MemoryFlags::Global | MemoryFlags::Writable, desc);

        assert_or(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP; #%us) for extending object pool "
              "%Xp (%us, %us, %us, %us, %us): %H."
            , vaddr + i * PageSize, paddr, i, pool
            , objectSize, headerSize, minimumExtraObjects, oldPageCount, newPageCount
            , res)
        {
            Cpu::GetData()->DomainDescriptor->PhysicalAllocator->FreePageAtAddress(paddr);
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

Handle ReleasePoolTest(size_t objectSize, size_t headerSize, ObjectPoolBase * pool)
{
    askedToRemove = true;

    // msg_("~~ ASKED TO REMOVE POOL %Xp ~~%n"
    //     , pool);

    //  A nice procedure here is to unmap the pool's pages one-by-one, starting
    //  from the highest. The kernel heap cursor will be pulled back if possible.

    Handle res;
    PageDescriptor * desc = nullptr;

    bool decrementedHeapCursor = true;
    //  Initial value is for simplifying the algorithm below.

    size_t const pageCount = RoundUp(objectSize * pool->Capacity + headerSize, PageSize) / PageSize;

    vaddr_t vaddr = (vaddr_t)pool + (pageCount - 1) * PageSize;
    size_t i = pageCount;

    /*msg("~~ PC=%us, vaddr=%Xp ~~%n", pageCount, vaddr);//*/

    for (/* nothing */; i > 0; --i, vaddr -= PageSize)
    {
        /*msg("~~ UNMAPPING %Xp FROM POOL %Xp;", vaddr, pool);//*/

        res = Vmm::UnmapPage(&BootstrapProcess, vaddr, desc);

        assert_or(res.IsOkayResult()
            , "Failed to unmap page #%us from pool %Xp.%n"
            , i, pool)
        {
            break;

            //  The rest of the function will attempt to adapt.
        }

        /*if (desc != nullptr)
        {
            msg(" PHYSPAGE ");
            desc->PrintToTerminal(Debug::DebugTerminal);
            msg(" ~~%n");
        }//*/

        vaddr_t expectedCursor = vaddr + PageSize;

        if (decrementedHeapCursor)
            decrementedHeapCursor = Vmm::KernelHeapCursor.CmpXchgStrong(expectedCursor, vaddr);
    }

    //  TODO: Salvage pool when it failed to remove all pages.
    //if (i > 0 && i < pageCount)

    return i < pageCount
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

Handle ObjectAllocatorSpamTest()
{
    Handle res;

    ObjectAllocatorTestBarrier1.Reach();

    size_t volatile freeCount1 = testAllocator.GetFreeCount();
    size_t volatile capacity1 = testAllocator.GetCapacity();
    size_t volatile busyCount1 = testAllocator.GetBusyCount();

    ObjectAllocatorTestBarrier2.Reach();
    ObjectAllocatorTestBarrier1.Reset(); //  Prepare the first barrier for re-use.

#ifdef __BEELZEBUB__PROFILE
    uint64_t perfAcc = 0;
#endif

    for (size_t x = REPETITION_COUNT; x > 0; --x)
    {
#ifdef __BEELZEBUB__PROFILE
        uint64_t perfStart = CpuInstructions::Rdtsc();
#endif

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

#ifdef __BEELZEBUB__PROFILE
        uint64_t perfEnd = CpuInstructions::Rdtsc();

        perfAcc += perfEnd - perfStart;
#endif
    }

#ifdef __BEELZEBUB__PROFILE
    MSG_("Core %us: %u8 / %us = %u8; %u8 / %us = %u8; %n"
        , Cpu::GetData()->Index
        , perfAcc, REPETITION_COUNT, perfAcc / REPETITION_COUNT
        , perfAcc, REPETITION_COUNT_3, perfAcc / REPETITION_COUNT_3);
#endif

    ObjectAllocatorTestBarrier3.Reach();
    ObjectAllocatorTestBarrier2.Reset(); //  Prepare the second barrier for re-use.

    ASSERT(capacity1 - freeCount1 == testAllocator.GetCapacity() - testAllocator.GetFreeCount()
        , "Allocator's deduced busy object count has a shady value: %us (%us - %us)"
          ", expected %us (%us - %us)."
        , testAllocator.GetCapacity() - testAllocator.GetFreeCount()
        , testAllocator.GetCapacity(), testAllocator.GetFreeCount()
        , capacity1 - freeCount1, capacity1, freeCount1);

    ASSERT(busyCount1 == testAllocator.GetBusyCount()
        , "Allocator's busy object count has a shady value: %us, expected %us."
        , busyCount1, testAllocator.GetBusyCount());

    ObjectAllocatorTestBarrier1.Reach();
    ObjectAllocatorTestBarrier3.Reset(); //  Prepare the third barrier for re-use.

    return HandleResult::Okay;
}

__noinline Handle ThreePoolTest()
{
    Handle res;

    for (size_t testCnt = 0; testCnt < 1000; ++testCnt)
    {
        askedToAcquire = askedToRemove = askedToEnlarge = false;

        /*msg("~~ ACQUIRING FIRST OBJECT ~~%n");//*/
        COMPILER_MEMORY_BARRIER();

        TESTALLOC3(TestStructure, m)

        COMPILER_MEMORY_BARRIER();

        ASSERT(askedToAcquire
            , "The allocator wasn't asked to acquire a pool when allocating a "
              "(new) first object..?");

        ASSERT(testAllocator.PoolCount == 1
            , "Test allocator should have exactly one pool now, not %us.%n"
            , testAllocator.PoolCount.Load());

        /*msg("~~ STARTING MULTIPLE POOL TEST WITH cap %us (%u4), fc %us (%u4) ~~%n"
            , testAllocator.GetCapacity(), testAllocator.FirstPool->Capacity
            , testAllocator.GetFreeCount(), testAllocator.FirstPool->FreeCount);//*/
        COMPILER_MEMORY_BARRIER();

        tOm->Next = nullptr;
        //  This field has to be initialized.

        askedToAcquire = false;

        TestStructure * tOx = tOm, * tOy, * tOt;
        bool allocatedOnce = false;
        size_t capacity2 = 13379001;

        for (size_t i = testAllocator.GetFreeCount() + 2; i > 0; --i)
        {
            tOy = tOx;

            if unlikely(i == 3)
            {
                ASSERT(testAllocator.GetFreeCount() == 1
                    , "Test allocator should only have one free object, not %us.%n"
                    , testAllocator.GetFreeCount());

                ASSERT(testAllocator.PoolCount == (allocatedOnce ? 2u : 1u)
                    , "Test allocator should have exactly %u4 pool now, not %us.%n"
                    , allocatedOnce ? 2u : 1u, testAllocator.PoolCount.Load());

                ASSERT(!askedToAcquire
                    , "The allocator asked to acquire a pool before it was full..?");

                ASSERT(!askedToEnlarge
                    , "The allocator asked to enlarge a pool before it was full..?");

                canEnlarge = false;
            }
            else if unlikely(i == 2)
            {
                ASSERT(testAllocator.GetFreeCount() == 0
                    , "Test allocator should only have 0 free objects, not %us.%n"
                    , testAllocator.GetFreeCount());

                ASSERT(testAllocator.PoolCount == (allocatedOnce ? 2u : 1u)
                    , "Test allocator should have exactly %u4 pool now, not %us.%n"
                    , allocatedOnce ? 2u : 1u, testAllocator.PoolCount.Load());

                ASSERT(!askedToAcquire
                    , "The allocator asked to acquire a pool before it was full..?");

                ASSERT(!askedToEnlarge
                    , "The allocator asked to enlarge a pool before it was full..?");

                canEnlarge = false;

                /*msg("~~ NEXT POOL SHOULD BE ACQUIRED NAO! ~~%n");//*/
            }

            res = testAllocator.AllocateObject(tOx);

            ASSERT(res.IsOkayResult()
                , "Failed to allocate capacity-filling object #%us: %H%n"
                , i, res);

            ASSERT(0 != (tOx->Qwords[0] & 1)
                , "Recently-allocated object %Xp has busy bit clear!%n"
                , tOx);

            tOt = tOy;
            while (tOt != nullptr)
            {
                TESTDIFF(x, t);

                tOt = tOt->Next;
            }

            if (tOy != nullptr)
                ASSERT(tOy->Next != tOx
                    , "Previous test object points to the current one..?");

            tOx->Next = tOy;

            if unlikely(i == 1 && !allocatedOnce)
            {
                ASSERT(testAllocator.GetFreeCount() > 1
                    , "Test allocator should have more than one free object!%n");

                ASSERT(testAllocator.PoolCount == 2
                    , "Test allocator should have exactly two pools now, not %us.%n"
                    , testAllocator.PoolCount.Load());

                i = testAllocator.GetFreeCount() + 3;
                allocatedOnce = true;

                /*msg("~~ i = %us ~~%n", i);//*/

                ASSERT(askedToAcquire
                    , "The allocator should have been asked to acquire a pool "
                      "when the first one got full and couldn't enlarge.");

                ASSERT(!askedToEnlarge
                    , "The allocator apparently enlarged the first pool when it "
                      "was full..?");

                capacity2 = testAllocator.GetCapacity();

                askedToAcquire = false;
            }
        }

        ASSERT(capacity2 != testAllocator.GetCapacity()
            , "The allocator's capacity should've changed from %us!"
            , capacity2);

        ASSERT(askedToAcquire
            , "The allocator should have been asked to acquire a pool "
              "when the second one got full and couldn't enlarge.");

        ASSERT(!askedToEnlarge
            , "The allocator apparently enlarged the second pool when it "
              "was full..?");

        askedToAcquire = false;

        ASSERT(testAllocator.PoolCount == 3
            , "Test allocator should have exactly three pools now, not %us.%n"
            , testAllocator.PoolCount.Load());

        //  Now clean up the acquired objects.

        size_t poolCount = testAllocator.PoolCount.Load();

        while (tOx != nullptr)
        {
            auto next = tOx->Next;

            TESTREMOV3(x)
            //  Simple, huh?

            size_t newPoolCount = testAllocator.PoolCount.Load();

            if (poolCount == newPoolCount)
                ASSERT(0 == (tOx->Qwords[0] & 1)
                    , "Recently-deallocated object %Xp has busy bit set!%n"
                    , tOx);
            else
            {
                //  The pool containing the previous object was removed and,
                //  thus cannot be accessed anymore.

                poolCount = newPoolCount;
            }

            tOx = next;
        }

        /*msg("~~ FINISHED REMOVING LAST OBJECT ~~%n");//*/
        COMPILER_MEMORY_BARRIER();

        ASSERT(askedToRemove
            , "The allocator should have asked to remove a pool after "
              "deallocating the last object!");

        ASSERT(testAllocator.GetFreeCount() == 0
            , "The test allocator should have a free count of 0, not %us!"
            , testAllocator.GetFreeCount());

        ASSERT(testAllocator.PoolCount == 0
            , "Test allocator should have no pools now, not %us.%n"
            , testAllocator.PoolCount.Load());

        ASSERT(testAllocator.GetCapacity() == 0
            , "The test allocator should have a capacity of 0, not %us!"
            , testAllocator.GetCapacity());

        ASSERT(testAllocator.FirstPool == nullptr
            , "Test allocator should have no first pool now, not %Xp.%n"
            , testAllocator.FirstPool);
    }

    return HandleResult::Okay;
}

Handle ObjectAllocatorParallelAcquireTest()
{
    Handle res;

    size_t const objCount = 2 * PageSize / sizeof(TestStructure);
    //  Should make 3 merry pools.

    ObjectAllocatorTestBarrier2.Reach();
    ObjectAllocatorTestBarrier1.Reset(); //  Prepare the first barrier for re-use.

    /*msg_("Core #%us: Gunna allocate %us objects.%n"
        , System::Cpu::GetData()->Index, objCount);//*/

    ObjectAllocatorTestBarrier3.Reach();
    ObjectAllocatorTestBarrier2.Reset(); //  Prepare the second barrier for re-use.

    TestStructure * tOx = nullptr, * tOy = nullptr;

    for (size_t i = 0; i < objCount; ++i)
    {
        tOy = tOx;

        /*msg_("Core #%us: Gunna allocate object #%us.%n"
            , System::Cpu::GetData()->Index, i);//*/

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

    ObjectAllocatorTestBarrier1.Reach();
    ObjectAllocatorTestBarrier3.Reset(); //  Prepare the third barrier for re-use.

    ASSERT((testAllocator.GetCapacity() - testAllocator.GetFreeCount()) % objCount == 0
        , "Busy object count should be a multiple of %us, but %us (%% %us) is not.%n"
        , objCount, testAllocator.GetCapacity() - testAllocator.GetFreeCount()
        , (testAllocator.GetCapacity() - testAllocator.GetFreeCount()) % objCount);

    ObjectAllocatorTestBarrier2.Reach();
    ObjectAllocatorTestBarrier1.Reset(); //  Prepare the first barrier for re-use.

    while (tOx != nullptr)
    {
        auto next = tOx->Next;

        TESTREMOV3(x)
        //  Simple, huh?

        tOx = next;
    }

    ObjectAllocatorTestBarrier3.Reach();
    ObjectAllocatorTestBarrier2.Reset(); //  Prepare the second barrier for re-use.

    return HandleResult::Okay;
}

Handle TestObjectAllocator(bool const bsp)
{
    Handle res;

    /*
        So, after I enabled scheduling during tests, the object allocator test
        started failing randomly. After 2.5 days (magic number for me) of testing,
        I realized it happens because a pool fails to enlarge on demand. :L
        Since that functionality is explicitly tested and expected, things started
        catching fire. To prevent this, interrupts are now disabled before the
        pool that needs enlarging is allocated.
     */

    if likely(!bsp)
    {
        res = ObjectAllocatorSpamTest();

        if (!res.IsOkayResult())
            return res;

        //  The BSP will do more magic between these tests.

        return ObjectAllocatorParallelAcquireTest();
    }
    else
    {
        askedToAcquire = askedToRemove = false;
        canEnlarge = true;

        new (&testAllocator) ObjectAllocatorSmp(sizeof(TestStructure), __alignof(TestStructure)
            , &AcquirePoolTest, &EnlargePoolTest, &ReleasePoolTest
            , PoolReleaseOptions::ReleaseAll, 0, SIZE_MAX);

        /*msg("Test allocator (%Xp): Capacity = %Xs, Free Count = %Xs, Pool Count = %Xs;%n"
            , &testAllocator
            , testAllocator.Capacity.Load()
            , testAllocator.FreeCount.Load()
            , testAllocator.PoolCount.Load());//*/

        InterruptGuard<false> ig;

        TESTALLOC3(TestStructure, 1)

        ASSERT(askedToAcquire
            , "The allocator wasn't asked to acquire a pool when allocating the "
              "first object..?");

        askedToAcquire = false;

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

        /*msg("Test allocator (%Xp): Capacity = %us, Free Count = %us, Pool Count = %us;%n"
            , &testAllocator
            , testAllocator.Capacity.Load()
            , testAllocator.FreeCount.Load()
            , testAllocator.PoolCount.Load());//*/

        ASSERT(testAllocator.PoolCount == 1
            , "Test allocator should have only one pool, not %us.%n"
            , testAllocator.PoolCount.Load());

        ASSERT(testAllocator.GetCapacity() - testAllocator.GetFreeCount() == 4
                && testAllocator.GetBusyCount() == 4
            , "Test allocator should have only 4 objects non-free, not %us "
              "(%us - %us).%n"
            , testAllocator.GetCapacity() - testAllocator.GetFreeCount()
            , testAllocator.GetCapacity(), testAllocator.GetFreeCount());

        ASSERT(!askedToAcquire
            , "The allocator was asked to acquire a pool when it shouldn't have "
              "filled any pools yet!");

        res = ObjectAllocatorSpamTest();

        if (!res.IsOkayResult())
            return res;

        askedToEnlarge = askedToAcquire = askedToRemove = false;
        //  These may have occured already!

        TestStructure * tOx = nullptr, * tOy = nullptr, * tOt = nullptr;

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

            tOt = tOy;
            while (tOt != nullptr)
            {
                TESTDIFF(x, t);

                tOt = tOt->Next;
            }

            if (tOy != nullptr)
                ASSERT(tOy->Next != tOx
                    , "Previous test object points to the current one..?");

            tOx->Next = tOy;
        }

        ASSERT(askedToEnlarge
            , "The allocator should have asked to enlarge a pool after "
              "allocating the last object!");

        ASSERT(!askedToAcquire
            , "The allocator was asked to acquire a pool when it already has "
              "asked to enlarge..??");

        //  Now let's allocate moar objects!

        TESTALLOC3(TestStructure, 11)
        TESTALLOC3(TestStructure, 12)
        TESTALLOC3(TestStructure, 13)
        TESTALLOC3(TestStructure, 14)

        TESTREMOV3(13)

        ASSERT(!askedToRemove
            , "The allocator asked to remove a pool when objects should still be"
              "left in it!");

        res = testAllocator.DeallocateObject(tO13);
        ASSERT(res.IsResult(HandleResult::ObjaAlreadyFree)
            , "Deletion of object \"13\" (%Xp) should've returned "
              "\"already freed\": %H%n"
            , tO13, res);

        ASSERT(!askedToRemove
            , "The allocator asked to remove a pool when objects should still be"
              "left in it!");

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

        ASSERT(!askedToRemove
            , "The allocator asked to remove a pool when objects should still be"
              "left in it!");

        //  This uses `y` and not `x` because the latter is removed last.
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

        ASSERT(testAllocator.GetCapacity() == testAllocator.FirstPool->Capacity
            , "The test allocator should have a capacity equal to its first "
              "pool's, not %us (expected %us)!"
            , testAllocator.GetCapacity(), testAllocator.FirstPool->Capacity);

        ASSERT(testAllocator.GetFreeCount() == testAllocator.FirstPool->FreeCount
            , "The test allocator should have a free count equal to its first "
              "pool's, not %us (expected %us)!"
            , testAllocator.GetFreeCount(), testAllocator.FirstPool->FreeCount);

        ASSERT(testAllocator.GetCapacity() - testAllocator.GetFreeCount() == 1
            , "The test allocator should have exactly one deduced busy object, "
              "not %us (%us - %us)!"
            , testAllocator.GetCapacity() - testAllocator.GetFreeCount()
            , testAllocator.GetCapacity(), testAllocator.GetFreeCount());

        ASSERT(testAllocator.GetBusyCount() == 1
            , "The test allocator should have exactly one busy object, not %us!"
            , testAllocator.GetBusyCount());

        /*msg("~~ REMOVING LAST OBJECT ~~%n");//*/
        COMPILER_MEMORY_BARRIER();

        TESTREMOV3(x)

        /*msg("~~ FINISHED REMOVING LAST OBJECT ~~%n");//*/
        COMPILER_MEMORY_BARRIER();

        ASSERT(askedToRemove
            , "The allocator should have asked to remove a pool after "
              "deallocating the last object!");

        ASSERT(testAllocator.GetCapacity() == 0
            , "The test allocator should have a capacity of 0, not %us!"
            , testAllocator.GetCapacity());

        ASSERT(testAllocator.GetFreeCount() == 0
            , "The test allocator should have a free count of 0, not %us!"
            , testAllocator.GetFreeCount());

        ASSERT(testAllocator.PoolCount == 0
            , "Test allocator should have no pools now, not %us.%n"
            , testAllocator.PoolCount.Load());

        ASSERT(testAllocator.FirstPool == nullptr
            , "Test allocator should have no first pool now, not %Xp.%n"
            , testAllocator.FirstPool);

        //  Now let's try getting three pools!

        res = ThreePoolTest();

        if (!res.IsOkayResult())
            return res;

        //  Now parallel allocations should test that pool acquisition doesn't mess up.

        res = ObjectAllocatorParallelAcquireTest();

        if (!res.IsOkayResult())
            return res;

        return HandleResult::Okay;
    }
}

#endif
