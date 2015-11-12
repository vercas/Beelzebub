#include <memory/manager_amd64.hpp>
#include <system/cpu.hpp>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Memory;

static __bland inline void Lock(MemoryManagerAmd64 & mm, const vaddr_t vaddr, const bool alloc = false)
{
    if (vaddr < VirtualAllocationSpace::LowerHalfEnd)
        mm.UserLock.Acquire();
    else if (vaddr >= VirtualAllocationSpace::HigherHalfStart)
    {
        if (vaddr <= MemoryManagerAmd64::KernelModulesEnd)
            MemoryManagerAmd64::KernelModulesLock.Acquire();
        else if (vaddr <= MemoryManagerAmd64::PasDescriptorsEnd)
            MemoryManagerAmd64::PasDescriptorsLock.Acquire();
        else if (vaddr <= MemoryManagerAmd64::HandleTablesEnd)
            MemoryManagerAmd64::HandleTablesLock.Acquire();
        else if (vaddr <= MemoryManagerAmd64::KernelHeapEnd)
        {
            /*if (alloc || (vaddr >= Cpu::GetKernelHeapStart() && vaddr < Cpu::GetKernelHeapEnd()))
            {
                MemoryManagerAmd64::KernelHeapMasterLock.Await();
                //  The master lock must be free!

                __sync_add_and_fetch(&MemoryManagerAmd64::KernelHeapLockCount, 1);
                Cpu::GetKernelHeapSpinlock()->Acquire();
                //  Increment the number of heap locks and acquire this CPU's heap lock.
            }
            else
            {*/
                MemoryManagerAmd64::KernelHeapMasterLock.Acquire();

                while (MemoryManagerAmd64::KernelHeapLockCount > 0)
                {
                    asm volatile("pause");

                    //  Yeah...
                }
            //}
        }
        else
            MemoryManagerAmd64::KernelBinariesLock.Acquire();
    }
}

static __bland inline void Unlock(MemoryManagerAmd64 & mm, const vaddr_t vaddr, const bool alloc = false)
{
    if (vaddr < VirtualAllocationSpace::LowerHalfEnd)
        mm.UserLock.Release();
    else if (vaddr >= VirtualAllocationSpace::HigherHalfStart)
    {
        if (vaddr <= MemoryManagerAmd64::KernelModulesEnd)
            MemoryManagerAmd64::KernelModulesLock.Release();
        else if (vaddr <= MemoryManagerAmd64::PasDescriptorsEnd)
            MemoryManagerAmd64::PasDescriptorsLock.Release();
        else if (vaddr <= MemoryManagerAmd64::HandleTablesEnd)
            MemoryManagerAmd64::HandleTablesLock.Release();
        else if (vaddr <= MemoryManagerAmd64::KernelHeapEnd)
        {
            /*if (alloc || (vaddr >= Cpu::GetKernelHeapStart() && vaddr < Cpu::GetKernelHeapEnd()))
            {
                __sync_sub_and_fetch(&MemoryManagerAmd64::KernelHeapLockCount, 1);
                Cpu::GetKernelHeapSpinlock()->Release();
                //  Decrement the number of heap locks and release this CPU's heap lock.
            }
            else*/
                MemoryManagerAmd64::KernelHeapMasterLock.Release();
        }
        else
            MemoryManagerAmd64::KernelBinariesLock.Release();
    }
}

/********************************
    MemoryManagerAmd64 struct
********************************/

/*  Statics  */

void MemoryManager::Initialize()
{

}

Atomic<vaddr_t> MemoryManagerAmd64::KernelModulesCursor {MemoryManagerAmd64::KernelModulesStart};
Spinlock<> MemoryManagerAmd64::KernelModulesLock;

Atomic<vaddr_t> MemoryManagerAmd64::PasDescriptorsCursor {MemoryManagerAmd64::PasDescriptorsStart};
Spinlock<> MemoryManagerAmd64::PasDescriptorsLock;

Spinlock<> MemoryManagerAmd64::HandleTablesLock;

Atomic<vaddr_t> MemoryManagerAmd64::KernelHeapCursor {MemoryManagerAmd64::KernelHeapStart};
size_t volatile MemoryManagerAmd64::KernelHeapLockCount = 0;
Spinlock<> MemoryManagerAmd64::KernelHeapMasterLock;

Spinlock<> MemoryManagerAmd64::KernelBinariesLock;

/*  Status  */

Handle MemoryManager::Activate()
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    mm.Vas->Activate();

    return Handle(HandleResult::Okay);
}

Handle MemoryManager::Switch(MemoryManager * const other)
{
    //MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    //  No de-activation required.

    return other->Activate();
}

bool MemoryManager::IsActive()
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    return mm.Vas->IsLocal();
}

/*  Page Management  */

Handle MemoryManager::MapPage(const vaddr_t vaddr, const paddr_t paddr, const PageFlags flags, PageDescriptor * const desc)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    PageDescriptor * pml3desc = nullptr;
    PageDescriptor * pml2desc = nullptr;
    PageDescriptor * pml1desc = nullptr;

    Lock(mm, vaddr);

    Handle res = mm.Vas->Map(vaddr, paddr, flags, pml3desc, pml2desc, pml1desc);

    Unlock(mm, vaddr);

    if likely(desc != nullptr)
        desc->IncrementReferenceCount();
    //  The page doesn't have to be in an allocation space, in which case the
    //  descriptor is null.

    if unlikely(pml3desc != nullptr) pml3desc->IncrementReferenceCount();
    if unlikely(pml2desc != nullptr) pml2desc->IncrementReferenceCount();
    if unlikely(pml1desc != nullptr) pml1desc->IncrementReferenceCount();

    return res;
}

Handle MemoryManager::MapPage(const vaddr_t vaddr, const paddr_t paddr, const PageFlags flags)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    PageDescriptor * pml3desc = nullptr;
    PageDescriptor * pml2desc = nullptr;
    PageDescriptor * pml1desc = nullptr;

    Lock(mm, vaddr);

    Handle res = mm.Vas->Map(vaddr, paddr, flags, pml3desc, pml2desc, pml1desc);

    Unlock(mm, vaddr);

    if unlikely(pml3desc != nullptr) pml3desc->IncrementReferenceCount();
    if unlikely(pml2desc != nullptr) pml2desc->IncrementReferenceCount();
    if unlikely(pml1desc != nullptr) pml1desc->IncrementReferenceCount();

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

    Lock(mm, vaddr);

    Handle res = mm.Vas->Unmap(vaddr, paddr);

    Unlock(mm, vaddr);

    //  TODO: Broadcast this unmapping if necessary..?

    PageDescriptor * desc;

    if (mm.Vas->Allocator->TryGetPageDescriptor(paddr, desc))
    {
        auto refcnt = desc->DecrementReferenceCount();

        mm.Vas->Allocator->FreePageAtAddress(paddr);
    }

    //  TODO: The page may be in another domain's allocator.
    //  That needs to be handled!

    return res;
}

Handle MemoryManager::TryTranslate(vaddr_t const vaddr, paddr_t & paddr)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    Pml1Entry * e;

    Handle res = mm.Vas->GetEntry(vaddr, e, false);

    if (res.IsOkayResult())
        paddr = e->GetAddress();
    else
        paddr = nullpaddr;

    return res;
}

Handle MemoryManager::AllocatePages(const size_t count, const AllocatedPageType type, const PageFlags flags, vaddr_t & vaddr)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    /*  Allocation cursor  */

    vaddr_t cursor;

    if (0 != (type & AllocatedPageType::VirtualUser))
    {
        cursor = mm.UserHeapCursor;

        mm.UserLock.Acquire();

        mm.UserHeapCursor += count << 12;

        if (mm.UserHeapCursor >= VirtualAllocationSpace::LowerHalfEnd)
            mm.UserHeapCursor -= VirtualAllocationSpace::LowerHalfEnd - (1 << 12);

        mm.UserLock.Release();
    }
    else
    {
        cursor = MemoryManagerAmd64::KernelHeapCursor;

        MemoryManagerAmd64::KernelHeapMasterLock.Acquire();

        MemoryManagerAmd64::KernelHeapCursor += count << 12;

        if (MemoryManagerAmd64::KernelHeapCursor >= MemoryManagerAmd64::KernelHeapEnd)
            MemoryManagerAmd64::KernelHeapCursor -= MemoryManagerAmd64::KernelHeapEnd;

        MemoryManagerAmd64::KernelHeapMasterLock.Release();
    }

    /*  Iteration  */

    /*VirtualAllocationSpace::Iterator it;

    Handle res = mm.Vas->GetIterator(it, cursor);

    for (size_t i = 0; i < count; ++i, ++it)
    {
        
        PageDescriptor * pml3desc = nullptr;
        PageDescriptor * pml2desc = nullptr;
        PageDescriptor * pml1desc = nullptr;

    
        Lock(mm, vaddr);

        Handle res = mm.Vas->Map(vaddr, paddr, flags, pml3desc, pml2desc, pml1desc);

        Unlock(mm, vaddr);

        if unlikely(pml3desc != nullptr) pml3desc->IncrementReferenceCount();
        if unlikely(pml2desc != nullptr) pml2desc->IncrementReferenceCount();
        if unlikely(pml1desc != nullptr) pml1desc->IncrementReferenceCount();

        PageDescriptor * desc;

        if (mm.Vas->Allocator->TryGetPageDescriptor(paddr, desc))
            desc->IncrementReferenceCount();
        //  The page doesn't have to be in an allocation space.

        if (!res.IsOkayResult())
            return res;
    }

    return res; //*/
}

Handle MemoryManager::FreePages(const vaddr_t vaddr, const size_t count)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);
}

/*  Flags  */

Handle MemoryManager::GetPageFlags(const vaddr_t vaddr, PageFlags & flags)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    Lock(mm, vaddr);

    Handle res = mm.Vas->GetPageFlags(vaddr, flags);

    Unlock(mm, vaddr);

    return res;
}

Handle MemoryManager::SetPageFlags(const vaddr_t vaddr, const PageFlags flags)
{
    MemoryManagerAmd64 & mm = *((MemoryManagerAmd64 *)this);

    Lock(mm, vaddr);

    Handle res = mm.Vas->SetPageFlags(vaddr, flags);

    Unlock(mm, vaddr);

    return res;
}
