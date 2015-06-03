#include <memory/virtual_allocator.hpp>
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

    paddr_t pml4_paddr = this->Allocator->AllocatePage();

    if (pml4_paddr == nullptr)
        return Handle(HandleResult::OutOfMemory);
    
    Pml4 & pml4 = *((Pml4 *)pml4_paddr);
    //  Cheap.

    memset((void *)pml4_paddr, 0, 4096);
    //  Clear it all out!

    Cr3 cr3 = Cpu::GetCr3();
    Pml4 & currentPml4 = *cr3.GetPml4Ptr();

    for (uint16_t i = 0; i < 256; ++i)
        pml4[i] = currentPml4[i];
    //  Temporarily-preserved identity mapping.
    //  The first cloning will discard it.

    pml4[(uint16_t)511] = currentPml4[(uint16_t)511];

    for (uint16_t i = 256; i < AlienFractalIndex; ++i)
    {
        paddr_t pml3_paddr = this->Allocator->AllocatePage();

        memset((void *)pml3_paddr, 0, 4096);
        //  Clear again.

        pml4[i] = Pml4Entry(pml3_paddr, true, true, true, false);
    }

    pml4[LocalFractalIndex] = Pml4Entry(pml4_paddr, true, true, false, NX);
    pml4[AlienFractalIndex] = pml4[LocalFractalIndex];

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

    for (uint16_t i = 0; i < 256; ++i)
        pml4[i] = Pml4Entry();
    //  Getting rid of those naughty identity maps.

    //  Re-activate, to flush the identity maps.
    this->Activate();

    return Handle(HandleResult::Okay);
}

Handle VirtualAllocationSpace::Clone(VirtualAllocationSpace * const target)
{
    new (target) VirtualAllocationSpace(this->Allocator);

    paddr_t pml4_paddr = target->Pml4Address = this->Allocator->AllocatePage();

    if (pml4_paddr == nullptr)
        return Handle(HandleResult::OutOfMemory);
    
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

    return Handle(HandleResult::Okay);
}

/*  Translation  */

Handle VirtualAllocationSpace::TryTranslate(const vaddr_t vaddr, paddr_t & paddr)
{
    if unlikely((vaddr >= FractalStart && vaddr < FractalEnd     )
             || (vaddr >= LowerHalfEnd && vaddr < HigherHalfStart))
        return Handle(HandleResult::PageMapIllegalRange);

    paddr_t offset = vaddr & 0xFFFULL;

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

    if likely(pml1[ind].GetPresent())
    {
        paddr = pml1[ind].GetAddress() + offset;

        return Handle(HandleResult::Okay);
    }
    else
        return Handle(HandleResult::PageUnmapped);
}

/*  Mapping  */

Handle VirtualAllocationSpace::Map(const vaddr_t vaddr, const paddr_t paddr, const PageFlags flags)
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

    if (!pml4[ind].GetPresent())
    {
        assert(pml4[ind].IsNull()
            , "Absent PML4 entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        const paddr_t newPml3 = this->Allocator->AllocatePage();

        if (newPml3 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml4[ind] = Pml4Entry(newPml3, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml3p, 0, 4096);
    }

    Pml3 & pml3 = *pml3p;
    ind = GetPml3Index(vaddr);

    if (!pml3[ind].GetPresent())
    {
        assert(pml3[ind].IsNull()
            , "Absent PDPT entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        const paddr_t newPml2 = this->Allocator->AllocatePage();

        if (newPml2 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml3[ind] = Pml3Entry(newPml2, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml2p, 0, 4096);
    }
    
    Pml2 & pml2 = *pml2p;
    ind = GetPml2Index(vaddr);

    if (!pml2[ind].GetPresent())
    {
        assert(pml2[ind].IsNull()
            , "Absent PD entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        const paddr_t newPml1 = this->Allocator->AllocatePage();

        if (newPml1 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml2[ind] = Pml2Entry(newPml1, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml1p, 0, 4096);
    }
    
    Pml1 & pml1 = *pml1p;
    ind = GetPml1Index(vaddr);

    if (pml1[ind].GetPresent())
        return Handle(HandleResult::PageMapped);

    pml1[ind] = Pml1Entry(paddr, true
        , 0 != (flags & PageFlags::Writable)
        , 0 != (flags & PageFlags::Userland)
        , 0 != (flags & PageFlags::Global)
        , 0 == (flags & PageFlags::Executable));
    //  Present, writable, user-accessible, global, executable.

    return Handle(HandleResult::Okay);
}

Handle VirtualAllocationSpace::Unmap(const vaddr_t vaddr, paddr_t & res)
{
    if unlikely((vaddr >= FractalStart && vaddr < FractalEnd     )
             || (vaddr >= LowerHalfEnd && vaddr < HigherHalfStart))
        return Handle(HandleResult::PageMapIllegalRange);

    if unlikely((vaddr & 0xFFFULL) != 0)
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

    if likely(pml1[ind].GetPresent())
    {
        res = pml1[ind].GetAddress();
        pml1[ind] = Pml1Entry();
        //  Null.

        return Handle(HandleResult::Okay);
    }
    else
        return Handle(HandleResult::PageUnmapped);
}
