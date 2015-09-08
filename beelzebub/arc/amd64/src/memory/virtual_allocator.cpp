/**
 *  IMPORTANT NOTE:
 *  The Map function may allocate pages for page tables. If so, it will
 *  increment the reference count of those pages.
 *  However, it will NOT increment the reference count of the target page!
 *  That is up to the caller.
 */

#include <memory/virtual_allocator.hpp>
#include <memory/manager_amd64.hpp>
#include <system/cpuid.hpp>
#include <system/cpu.hpp>
#include <debug.hpp>
#include <string.h>
#include <math.h>
#include <cpp_support.h>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::Debug;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Memory::Paging;
using namespace Beelzebub::System;

/************************************
    VirtualAllocationSpace struct
************************************/

/*  Cached Feature Flags  */

bool VirtualAllocationSpace::Page1GB;
bool VirtualAllocationSpace::NX;

/*  Constructors    */

VirtualAllocationSpace::VirtualAllocationSpace(PageAllocator * const allocator)
    : Allocator( allocator )
    //, FreePagesCount(0)
    //, MappedPagesCount(0)
{

}

/*  Main Operations  */

Handle VirtualAllocationSpace::Bootstrap()
{
    //  The bootstrap function prepares the FIRST virtual allocation space
    //  for use. 'Tis necessary because of identity-mapping and the lack of
    //  a proper kernel space.

    VirtualAllocationSpace::Page1GB = BootstrapProcessorId.CheckFeature(CpuFeature::Page1GB);
    VirtualAllocationSpace::NX      = BootstrapProcessorId.CheckFeature(CpuFeature::NX);

    if (NX)
        Cpu::EnableNxBit();

    PageDescriptor * desc = nullptr;

    paddr_t pml4_paddr = this->Allocator->AllocatePage(desc);

    if (pml4_paddr == nullptr)
        return Handle(HandleResult::OutOfMemory);

    Pml4 & oldPml4 = *((Pml4 *)pml4_paddr);
    //  Cheap.

    memset((void *)pml4_paddr, 0, 4096);
    //  Clear it all out!
    desc->IncrementReferenceCount();
    //  Increment reference count...

    Cr3 cr3 = Cpu::GetCr3();
    Pml4 & currentPml4 = *cr3.GetPml4Ptr();

    for (uint16_t i = 0; i < 256; ++i)
        oldPml4[i] = currentPml4[i];
    //  Temporarily-preserved identity mapping.
    //  The first cloning will discard it.

    oldPml4[(uint16_t)511] = currentPml4[(uint16_t)511];

    for (uint16_t i = 256; i < AlienFractalIndex; ++i)
    {
        paddr_t pml3_paddr = this->Allocator->AllocatePage(desc);

        memset((void *)pml3_paddr, 0, 4096);
        //  Clear again.
        desc->IncrementReferenceCount();
        //  Increment reference count...

        oldPml4[i] = Pml4Entry(pml3_paddr, true, true, true, false);
    }

    oldPml4[LocalFractalIndex] = Pml4Entry(pml4_paddr, true, true, false, NX);
    oldPml4[AlienFractalIndex] = oldPml4[LocalFractalIndex];

    this->Pml4Address = pml4_paddr;

    //  Activation, to finish the process.
    this->Activate();

    //  Remapping PAS control structures.

    PageAllocationSpace * cur = this->Allocator->FirstSpace;
    bool pendingLinksMapping = true;
    vaddr_t curLoc = PasControlStructuresStart; //  Used for serial allocation.
    Handle res; //  Temporary result.

    do
    {
        if ((vaddr_t)cur < HigherHalfStart && pendingLinksMapping)
        {
            //msg("Mapping links from %XP to %Xp. ", (paddr_t)cur & ~0xFFFULL, curLoc);

            res = this->Map(curLoc, (paddr_t)cur & ~0xFFFULL, PageFlags::Global | PageFlags::Writable);
            //  Global because it's shared by processes, and writable for hotplug.

            assert(res.IsOkayResult()
                , "Failed to map links between allocation spaces: %H"
                , res);
            //  Failure is fatal.

            this->Allocator->RemapLinks((vaddr_t)cur & ~0xFFFULL, curLoc);
            //  Do the actual remapping.

            pendingLinksMapping = false;
            //  One page is the maximum.

            curLoc += 0x1000ULL;
            //  Increment the current location.
        }

        const paddr_t pasStart = cur->GetMemoryStart();
        const size_t controlStructuresSize = (cur->GetAllocationStart() - pasStart);
        //  Size of control pages.

        if (curLoc + controlStructuresSize > PasControlStructuresEnd)
            break;
        //  Well, we reached our maximum!

        for (size_t i = 0; i < controlStructuresSize; i += 4096)
        {
            res = this->Map(curLoc + i, pasStart + i, PageFlags::Global | PageFlags::Writable);

            assert(res.IsOkayResult()
                , "Failed to map page #%u8 (%Xp to %XP): %H"
                , i / 4096, curLoc + i, pasStart + i, res);
            //  Failure is fatal.
        }

        cur->RemapControlStructures(curLoc);
        //  Self-documented function name.

        curLoc += controlStructuresSize;

    } while ((cur = cur->Next) != nullptr);

    MemoryManagerAmd64::PasDescriptorsCursor = curLoc;

    Pml4 & pml4 = *GetLocalPml4();

    for (uint16_t i = 0; i < 256; ++i)
    {
        pml4[i] = Pml4Entry();
    }
    //  Getting rid of those naughty identity maps.

    //  Re-activate, to flush the identity maps.
    this->Activate();

    return Handle(HandleResult::Okay);
}

Handle VirtualAllocationSpace::Clone(VirtualAllocationSpace * const target)
{
    new (target) VirtualAllocationSpace(this->Allocator);

    PageDescriptor * desc;

    paddr_t pml4_paddr = target->Pml4Address = this->Allocator->AllocatePage(desc);

    if (pml4_paddr == nullptr)
        return Handle(HandleResult::OutOfMemory);

    desc->IncrementReferenceCount();
    //  Do the good deed.
    
    target->Alienate();
    //  So it can be accessible.

    Pml4 & pml4Local = *GetLocalPml4();
    Pml4 & pml4Alien = *GetAlienPml4();

    for (uint16_t i = 0; i < 256; ++i)
        pml4Alien[i] = Pml4Entry();
    //  Userland space will be empty.
    
    for (uint16_t i = 256; i < AlienFractalIndex; ++i)
        pml4Alien[i] = pml4Local[i];
    //  Kernel-specific tables.

    pml4Alien[LocalFractalIndex] = Pml4Entry(pml4_paddr, true, true, false, NX);

    pml4Alien[511] = pml4Local[511];
    //  Last page, where the kernel and bootloader-provided shenanigans sit
    //  snuggly together and drink hot cocoa.

    return Handle(HandleResult::Okay);
}

/*  Translation  */

template<typename cbk_t>
Handle VirtualAllocationSpace::TryTranslate(const vaddr_t vaddr, cbk_t cbk)
{
    if unlikely((vaddr >= FractalStart && vaddr < FractalEnd     )
             || (vaddr >= LowerHalfEnd && vaddr < HigherHalfStart))
        return Handle(HandleResult::PageMapIllegalRange);

    const bool nonLocal = (vaddr < LowerHalfEnd) && !this->IsLocal();
    uint16_t ind;   //  Used to hold the current index.

    Pml4 * pml4p; Pml3 * pml3p; Pml2 * pml2p; Pml1 * pml1p;

    if (nonLocal)
    {
        this->Alienate();

        pml4p = GetAlienPml4();
        pml3p = GetAlienPml3(vaddr);
        pml2p = GetAlienPml2(vaddr);
        pml1p = GetAlienPml1(vaddr);
    }
    else
    {
        pml4p = GetLocalPml4();
        pml3p = GetLocalPml3(vaddr);
        pml2p = GetLocalPml2(vaddr);
        pml1p = GetLocalPml1(vaddr);
    }

    Pml4 & pml4 = *pml4p;
    ind = GetPml4Index(vaddr);

    if unlikely(!pml4[ind].GetPresent())
        return Handle(HandleResult::PageUnmapped);

    Pml3 & pml3 = *pml3p;
    ind = GetPml3Index(vaddr);

    if unlikely(!pml3[ind].GetPresent())
        return Handle(HandleResult::PageUnmapped);
    
    Pml2 & pml2 = *pml2p;
    ind = GetPml2Index(vaddr);

    if unlikely(!pml2[ind].GetPresent())
        return Handle(HandleResult::PageUnmapped);
    
    Pml1 & pml1 = *pml1p;
    ind = GetPml1Index(vaddr);

    return cbk(pml1.Entries + ind);

    //  The status of the page is irrelevant.
}

/*  Mapping  */

Handle VirtualAllocationSpace::Map(const vaddr_t vaddr, const paddr_t paddr, const PageFlags flags, PageDescriptor * & pml3desc, PageDescriptor * & pml2desc, PageDescriptor * & pml1desc)
{
    if unlikely((vaddr >= FractalStart && vaddr < FractalEnd     )
             || (vaddr >= LowerHalfEnd && vaddr < HigherHalfStart))
        return Handle(HandleResult::PageMapIllegalRange);

    if unlikely((vaddr & 0xFFFULL) != 0 || (paddr & 0xFFFULL) != 0)
        return Handle(HandleResult::PageUnaligned);

    const bool nonLocal = (vaddr < LowerHalfEnd) && !this->IsLocal();
    uint16_t ind;   //  Used to hold the current index.

    Pml4 * pml4p; Pml3 * pml3p; Pml2 * pml2p; Pml1 * pml1p;

    if (nonLocal)
    {
        this->Alienate();

        pml4p = GetAlienPml4();
        pml3p = GetAlienPml3(vaddr);
        pml2p = GetAlienPml2(vaddr);
        pml1p = GetAlienPml1(vaddr);
    }
    else
    {
        pml4p = GetLocalPml4();
        pml3p = GetLocalPml3(vaddr);
        pml2p = GetLocalPml2(vaddr);
        pml1p = GetLocalPml1(vaddr);
    }

    Pml4 & pml4 = *pml4p;
    ind = GetPml4Index(vaddr);

    if unlikely(!pml4[ind].GetPresent())
    {
        assert(pml4[ind].IsNull()
            , "Absent PML4 entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        const paddr_t newPml3 = this->Allocator->AllocatePage(pml3desc);

        if (newPml3 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml4[ind] = Pml4Entry(newPml3, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml3p, 0, 4096);
    }

    Pml3 & pml3 = *pml3p;
    ind = GetPml3Index(vaddr);

    if unlikely(!pml3[ind].GetPresent())
    {
        assert(pml3[ind].IsNull()
            , "Absent PDPT entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        const paddr_t newPml2 = this->Allocator->AllocatePage(pml2desc);

        if (newPml2 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml3[ind] = Pml3Entry(newPml2, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml2p, 0, 4096);
    }
    
    Pml2 & pml2 = *pml2p;
    ind = GetPml2Index(vaddr);

    if unlikely(!pml2[ind].GetPresent())
    {
        assert(pml2[ind].IsNull()
            , "Absent PD entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        const paddr_t newPml1 = this->Allocator->AllocatePage(pml1desc);

        if (newPml1 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml2[ind] = Pml2Entry(newPml1, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml1p, 0, 4096);
    }
    
    Pml1 & pml1 = *pml1p;
    ind = GetPml1Index(vaddr);

    if unlikely(pml1[ind].GetPresent())
        return Handle(HandleResult::PageMapped);

    pml1[ind] = Pml1Entry(paddr, true
        , 0 != (flags & PageFlags::Writable)
        , 0 != (flags & PageFlags::Userland)
        , 0 != (flags & PageFlags::Global)
        , 0 == (flags & PageFlags::Executable) && NX);
    //  Present, writable, user-accessible, global, executable.

    return Handle(HandleResult::Okay);
}

Handle VirtualAllocationSpace::Map(const vaddr_t vaddr, const paddr_t paddr, const PageFlags flags)
{
    PageDescriptor * pml3desc = nullptr;
    PageDescriptor * pml2desc = nullptr;
    PageDescriptor * pml1desc = nullptr;

    Handle res = this->Map(vaddr, paddr, flags, pml3desc, pml2desc, pml1desc);

    if (pml3desc != nullptr) pml3desc->IncrementReferenceCount();
    if (pml2desc != nullptr) pml2desc->IncrementReferenceCount();
    if (pml1desc != nullptr) pml1desc->IncrementReferenceCount();

    return res;
}

Handle VirtualAllocationSpace::Unmap(const vaddr_t vaddr, paddr_t & paddr)
{
    return this->TryTranslate(vaddr, [&paddr](Pml1Entry * pE) __bland
        {
            if likely(pE->GetPresent())
            {
                paddr = pE->GetAddress();
                *pE = Pml1Entry();
                //  Null.

                return Handle(HandleResult::Okay);
            }
            else
            {
                paddr = 0;

                return Handle(HandleResult::PageUnmapped);
            }
        });
}

/*  Flags  */

Handle VirtualAllocationSpace::GetPageFlags(const vaddr_t vaddr, PageFlags & flags)
{
    return this->TryTranslate(vaddr, [&flags](Pml1Entry * pE) __bland
        {
            if likely(pE->GetPresent())
            {
                const Pml1Entry e = *pE;
                PageFlags f = PageFlags::None;

                if (  e.GetGlobal())    f |= PageFlags::Global;
                if (  e.GetUserland())  f |= PageFlags::Userland;
                if (  e.GetWritable())  f |= PageFlags::Writable;
                if (!(e.GetXd() && NX)) f |= PageFlags::Executable;

                flags = f;

                return Handle(HandleResult::Okay);
            }
            else
            {
                flags = PageFlags::None;

                return Handle(HandleResult::PageUnmapped);
            }
        });
}

Handle VirtualAllocationSpace::SetPageFlags(const vaddr_t vaddr, const PageFlags flags)
{
    return this->TryTranslate(vaddr, [flags](Pml1Entry * pE) __bland
        {
            if likely(pE->GetPresent())
            {
                Pml1Entry e = *pE;
        
                e.SetGlobal(  ((PageFlags::Global     & flags) != 0));
                e.SetUserland(((PageFlags::Userland   & flags) != 0));
                e.SetWritable(((PageFlags::Writable   & flags) != 0));
                e.SetXd( NX & ((PageFlags::Executable & flags) == 0));

                *pE = e;

                return Handle(HandleResult::Okay);
            }
            else
                return Handle(HandleResult::PageUnmapped);
        });
}

/**********************************************
    VirtualAllocationSpace::Iterator struct
**********************************************/

/*  Constructor(s)  */

Handle VirtualAllocationSpace::Iterator::Create(Iterator & dst, VirtualAllocationSpace * const space, const vaddr_t vaddr)
{
    if unlikely(0 != (vaddr & 0xFFF))
        return Handle(HandleResult::PageUnaligned);

    if unlikely((vaddr >= FractalStart && vaddr < FractalEnd     )
             || (vaddr >= LowerHalfEnd && vaddr < HigherHalfStart))
        return Handle(HandleResult::PageMapIllegalRange);

    dst = VirtualAllocationSpace::Iterator(space, vaddr);

    return dst.Initialize();
}

Handle VirtualAllocationSpace::Iterator::Initialize()
{
    const vaddr_t vaddr = this->VirtualAddress;
    const bool nonLocal = (vaddr < LowerHalfEnd) && !this->AllocationSpace->IsLocal();

    if (nonLocal)
        this->AllocationSpace->Alienate();

    /*Pml4 & pml4 = *(nonLocal ? GetAlienPml4() : GetLocalPml4());
    const uint16_t ind4 = GetPml4Index(vaddr);

    if likely(pml4[ind4].GetPresent())
    {
        Pml3 & pml3 = *(nonLocal ? GetAlienPml3(vaddr) : GetLocalPml3( vaddr ));
        const uint16_t ind3 = GetPml3Index(vaddr);
        
        if likely(pml3[ind3].GetPresent())
        {
            Pml2 & pml2 = *(nonLocal ? GetAlienPml2(vaddr) : GetLocalPml2( vaddr ));
            const uint16_t ind2 = GetPml2Index(vaddr);
        
            if likely(pml2[ind2].GetPresent())
            {
                Pml1 & pml1 = *(nonLocal ? GetAlienPml1(vaddr) : GetLocalPml1( vaddr ));
                const uint16_t ind1 = GetPml1Index(vaddr);
            
                this->Entry = &pml1[ind1];
            
                return HandleResult::Okay;
            }
        }
    }//*/

    auto const work = [vaddr, nonLocal](auto const & alien, auto const & local, auto const & index) __bland
    {
        auto & pml = *(nonLocal ? alien(vaddr) : local( vaddr ));
        return &pml[index(vaddr)];
    };

    if likely(work(&GetAlienPml4Ex, &GetLocalPml4Ex, &GetPml4Index)->GetPresent())
    {
        if likely(work(&GetAlienPml3, &GetLocalPml3, &GetPml3Index)->GetPresent())
        {
            if likely(work(&GetAlienPml2, &GetLocalPml2, &GetPml2Index)->GetPresent())
            {
                this->Entry = work(&GetAlienPml1, &GetLocalPml1, &GetPml1Index);

                return HandleResult::Okay;
            }
        }
    }

    this->Entry = nullptr;

    return Handle(HandleResult::PageUnmapped);
}

const VirtualAllocationSpace::Iterator & VirtualAllocationSpace::Iterator::operator +=(const VirtualAllocationSpace::Iterator::DifferenceType diff)
{
    const vaddr_t vaddr = this->VirtualAddress;
    const vaddr_t target = vaddr + (diff << 12);

    //  Must not go over the gap!

    assert(target < LowerHalfEnd || target > HigherHalfStart
        , "Paging table crawling iterator attempted to move over the void! (%Xp + %us pages = %Xp)"
        , vaddr, diff, target);

    //  A bit of over- and underflow checking.

    if (diff > 0)
        assert((vaddr_t)(diff << 12) < (~((vaddr_t)0) - vaddr)
        , "Paging table crawling iterator attempted to overflow! (%Xp + %us pages [%Xp])"
        , vaddr, diff, diff << 12);
    else if (diff < 0)
        assert(vaddr >= (vaddr_t)(-(diff << 12))
        , "Paging table crawling iterator attempted to underflow! (%Xp - %us pages [%Xp])"
        , vaddr, diff, diff << 12);
    else
        return *this;

    //  Set the new address, which is now valid, and initialize.

    this->VirtualAddress = target;
    this->Initialize();

    return *this;
}

const VirtualAllocationSpace::Iterator VirtualAllocationSpace::Iterator::operator +(const VirtualAllocationSpace::Iterator::DifferenceType diff)
{
    const vaddr_t vaddr = this->VirtualAddress;
    const vaddr_t target = vaddr + (diff << 12);

    //  Must not go over the gap!

    assert(target < LowerHalfEnd || target > HigherHalfStart
        , "Paging table crawling iterator attempted to move over the void! (%Xp + %us pages = %Xp)"
        , vaddr, diff, target);

    //  A bit of over- and underflow checking.

    if (diff > 0)
        assert((vaddr_t)(diff << 12) < (~((vaddr_t)0) - vaddr)
            , "Paging table crawling iterator attempted to overflow! (%Xp + %us pages [%Xp])"
            , vaddr, diff, diff << 12);
    else if (diff < 0)
        assert(vaddr >= (vaddr_t)(-(diff << 12))
            , "Paging table crawling iterator attempted to underflow! (%Xp - %us pages [%Xp])"
            , vaddr, diff, diff << 12);
    else
        return *this;

    //  Create new address space. :3

    Iterator other;

    Handle res = Create(other, this->AllocationSpace, target);

    assert(res.IsOkayResult()
        , "Failed to create new Iterator instance: %H. (%Xp - %us pages [%Xp])"
        , res, vaddr, diff, diff << 12);
    //  Must not fail.

    return other;
}

Handle VirtualAllocationSpace::Iterator::AllocateTables(PageDescriptor * & pml3desc, PageDescriptor * & pml2desc, PageDescriptor * & pml1desc)
{
    const vaddr_t vaddr = this->VirtualAddress;
    const bool nonLocal = (vaddr < LowerHalfEnd) && !this->AllocationSpace->IsLocal();
    uint16_t ind;   //  Used to hold the current index.

    Pml4 * pml4p; Pml3 * pml3p; Pml2 * pml2p; Pml1 * pml1p;

    if (nonLocal)
    {
        this->AllocationSpace->Alienate();

        pml4p = GetAlienPml4();
        pml3p = GetAlienPml3(vaddr);
        pml2p = GetAlienPml2(vaddr);
        pml1p = GetAlienPml1(vaddr);
    }
    else
    {
        pml4p = GetLocalPml4();
        pml3p = GetLocalPml3(vaddr);
        pml2p = GetLocalPml2(vaddr);
        pml1p = GetLocalPml1(vaddr);
    }

    Pml4 & pml4 = *pml4p;
    ind = GetPml4Index(vaddr);

    if unlikely(!pml4[ind].GetPresent())
    {
        assert(pml4[ind].IsNull()
            , "Absent PML4 entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        const paddr_t newPml3 = this->AllocationSpace->Allocator->AllocatePage(pml3desc);

        if (newPml3 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml4[ind] = Pml4Entry(newPml3, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml3p, 0, 4096);
    }

    Pml3 & pml3 = *pml3p;
    ind = GetPml3Index(vaddr);

    if unlikely(!pml3[ind].GetPresent())
    {
        assert(pml3[ind].IsNull()
            , "Absent PDPT entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        const paddr_t newPml2 = this->AllocationSpace->Allocator->AllocatePage(pml2desc);

        if (newPml2 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml3[ind] = Pml3Entry(newPml2, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml2p, 0, 4096);
    }

    Pml2 & pml2 = *pml2p;
    ind = GetPml2Index(vaddr);

    //  Yes, this one is likely to be absent.
    if likely(!pml2[ind].GetPresent())
    {
        assert(pml2[ind].IsNull()
            , "Absent PD entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        const paddr_t newPml1 = this->AllocationSpace->Allocator->AllocatePage(pml1desc);

        if (newPml1 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml2[ind] = Pml2Entry(newPml1, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml1p, 0, 4096);
    }

    Pml1 & pml1 = *pml1p;
    ind = GetPml1Index(vaddr);

    this->Entry = &pml1[ind];

    return Handle(HandleResult::Okay);
}
