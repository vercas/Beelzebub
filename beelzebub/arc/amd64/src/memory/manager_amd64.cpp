#include <memory/manager_amd64.hpp>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::Memory;

/********************************
    MemoryManagerAmd64 struct
********************************/

Spinlock MemoryManagerAmd64::KernelLock;

/*  Status  */

Handle MemoryManager::Activate()
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    mm.Vas->Activate();

    return Handle(HandleResult::Okay);
}

Handle MemoryManager::Switch(MemoryManager * const other)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    //  No de-activation required.

    return other->Activate();
}

bool MemoryManager::IsActive()
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    return mm.Vas->IsLocal();
}

/*  Page Management  */

Handle MemoryManager::MapPage(const vaddr_t vaddr, const paddr_t paddr, const PageFlags flags)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    if (vaddr < VirtualAllocationSpace::LowerHalfEnd)
        mm.UserLock.Acquire();
    else if (vaddr >= VirtualAllocationSpace::HigherHalfStart)
        mm.KernelLock.Acquire();

    Handle res = mm.Vas->Map(vaddr, paddr, flags);

    if (vaddr < VirtualAllocationSpace::LowerHalfEnd)
        mm.UserLock.Release();
    else if (vaddr >= VirtualAllocationSpace::HigherHalfStart)
        mm.KernelLock.Release();

    PageDescriptor * desc;

    if (mm.Vas->Allocator->TryGetPageDescriptor(paddr, desc))
        desc->IncrementReferenceCount();
    //  The page doesn't have to be in an allocation space.

    return res;
}

Handle MemoryManager::UnmapPage(const vaddr_t vaddr)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    paddr_t paddr;

    if (vaddr < VirtualAllocationSpace::LowerHalfEnd)
        mm.UserLock.Acquire();
    else if (vaddr >= VirtualAllocationSpace::HigherHalfStart)
        mm.KernelLock.Acquire();

    Handle res = mm.Vas->Unmap(vaddr, paddr);

    if (vaddr < VirtualAllocationSpace::LowerHalfEnd)
        mm.UserLock.Release();
    else if (vaddr >= VirtualAllocationSpace::HigherHalfStart)
        mm.KernelLock.Release();

    PageDescriptor * desc;

    if (mm.Vas->Allocator->TryGetPageDescriptor(paddr, desc))
        desc->DecrementReferenceCount();
    //  The page doesn't have to be in an allocation space.

    return res;
}

Handle MemoryManager::AllocatePages(const size_t count, const AllocatedPageType type, const PageFlags flags, vaddr_t & res)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);
}

Handle MemoryManager::FreePages(const vaddr_t vaddr, const size_t count)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);
}
