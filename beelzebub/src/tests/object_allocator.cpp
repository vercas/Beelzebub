#ifdef __BEELZEBUB__TEST_OBJA

#include <tests/object_allocator.hpp>
#include <memory/object_allocator.hpp>
#include <memory/page_allocator.hpp>
#include <memory/manager_amd64.hpp>
#include <kernel.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;

struct TestStructure
{
    uint64_t Qwords[3];
    uint32_t Dwords[3];
    uint16_t Words[3];
    uint16_t Bytes[3];
};

ObjectAllocator testAllocator;

__bland Handle AcquirePoolTest(size_t objectSize, size_t headerSize, size_t minimumObjects, ObjectPool * & result)
{
    size_t const pageCount = (objectSize + minimumObjects * headerSize + 0xFFF) / 0x1000;

    Handle res;
    PageDescriptor * desc = nullptr;
    //  Intermediate results.

    vaddr_t const vaddr = MemoryManagerAmd64::KernelHeapCursor.FetchAdd(pageCount * 0x1000);

    for (size_t i = 0; i < pageCount; ++i)
    {
        paddr_t const paddr = MainPageAllocator->AllocatePage(desc);
        //  Test page.

        assert(paddr != nullpaddr && desc != nullptr
            , "  Unable to allocate physical page #%us for an object pool (%us, %us, %us, %us)!"
            , i
            , objectSize, headerSize, minimumObjects, pageCount);

        desc->IncrementReferenceCount();

        res = BootstrapMemoryManager->MapPage(vaddr + i * 0x1000, paddr, PageFlags::Global | PageFlags::Writable);

        assert(res.IsOkayResult()
            , "  Failed to map page at %Xp (%XP; #%us) for an object pool (%us, %us, %us, %us): %H."
            , vaddr + i * 0x1000, paddr, i
            , objectSize, headerSize, minimumObjects, pageCount
            , res);

        if (!res.IsOkayResult())
            return res;
        //  Maybe the test is built in release mode.
    }

    return res;
}

__bland Handle EnlargePoolTest(size_t objectSize, size_t headerSize, size_t minimumExtraObjects, ObjectPool * pool)
{
    
}

__bland Handle ReleasePoolTest(size_t objectSize, size_t headerSize, ObjectPool * pool)
{

}

Handle TestObjectAllocator()
{
    new (&testAllocator) ObjectAllocator(sizeof(TestStructure), __alignof(TestStructure), &AcquirePoolTest, &EnlargePoolTest, &ReleasePoolTest, true);

    return HandleResult::Okay;
}

#endif
