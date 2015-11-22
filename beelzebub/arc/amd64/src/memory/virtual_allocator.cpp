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

/**
 *  IMPORTANT NOTE:
 *  The Map function may allocate pages for page tables. If so, it will
 *  increment the reference count of those pages.
 *  However, it will NOT increment the reference count of the target page!
 *  That is up to the caller.
 */

#include <memory/virtual_allocator.hpp>
#include <memory/manager_amd64.hpp>
#include <string.h>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

/************************************
    VirtualAllocationSpace struct
************************************/

/*  Cached Feature Flags  */

bool VirtualAllocationSpace::Page1GB;
bool VirtualAllocationSpace::NX;

/*  Main Operations  */

Handle VirtualAllocationSpace::Bootstrap(System::CpuId const * const bspcpuid)
{
    //  The bootstrap function prepares the FIRST virtual allocation space
    //  for use. 'Tis necessary because of identity-mapping and the lack of
    //  a proper kernel space.

    VirtualAllocationSpace::Page1GB = bspcpuid->CheckFeature(CpuFeature::Page1GB);
    VirtualAllocationSpace::NX      = bspcpuid->CheckFeature(CpuFeature::NX     );

    if (NX)
        Cpu::EnableNxBit();

    PageDescriptor * desc = nullptr;

    paddr_t const pml4_paddr = this->Allocator->AllocatePage(PageAllocationOptions::ThirtyTwoBit, desc);

    if (pml4_paddr == nullpaddr)
        return Handle(HandleResult::OutOfMemory);

    Pml4 & oldPml4 = *((Pml4 *)pml4_paddr);
    //  Cheap.

    memset((void *)pml4_paddr, 0, PageSize);
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

        memset((void *)pml3_paddr, 0, PageSize);
        //  Clear again.
        desc->IncrementReferenceCount();
        //  Increment reference count...

        oldPml4[i] = Pml4Entry(pml3_paddr, true, true, true, false);
    }

    oldPml4[LocalFractalIndex] = Pml4Entry(pml4_paddr, true, true, false, NX);
    oldPml4[AlienFractalIndex] = oldPml4[LocalFractalIndex];
    //  Very important note: the user-accessible bit is cleared.
    //  This means userland code will not be able to look at the fractal mapping.

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
            //msg("Mapping links from %XP to %Xp. ", RoundDown((paddr_t)cur, PageSize), curLoc);

            res = this->Map(curLoc, RoundDown((paddr_t)cur, PageSize), PageFlags::Global | PageFlags::Writable);
            //  Global because it's shared by processes, and writable for hotplug.

            ASSERT(res.IsOkayResult()
                , "Failed to map links between allocation spaces: %H"
                , res);
            //  Failure is fatal.

            this->Allocator->RemapLinks(RoundDown((vaddr_t)cur, PageSize), curLoc);
            //  Do the actual remapping.

            pendingLinksMapping = false;
            //  One page is the maximum.

            curLoc += PageSize;
            //  Increment the current location.
        }

        paddr_t const pasStart = cur->GetMemoryStart();
        size_t const controlStructuresSize = (cur->GetAllocationStart() - pasStart);
        //  Size of control pages.

        if (curLoc + controlStructuresSize > PasControlStructuresEnd)
            break;
        //  Well, the maximum is reached!

        for (size_t i = 0; i < controlStructuresSize; i += PageSize)
        {
            res = this->Map(curLoc + i, pasStart + i, PageFlags::Global | PageFlags::Writable);

            ASSERT(res.IsOkayResult()
                , "Failed to map page #%u8 (%Xp to %XP): %H"
                , i / PageSize, curLoc + i, pasStart + i, res);
            //  Failure is fatal.
        }

        cur->RemapControlStructures(curLoc);
        //  Self-documented function name.

        curLoc += controlStructuresSize;

    } while ((cur = cur->Next) != nullptr);

    MemoryManagerAmd64::PasDescriptorsCursor = curLoc;

    Pml4 & pml4 = *GetLocalPml4();

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

    PageDescriptor * desc;

    paddr_t const pml4_paddr = target->Pml4Address = this->Allocator->AllocatePage(desc);

    if (pml4_paddr == nullpaddr)
        return HandleResult::OutOfMemory;

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

    return HandleResult::Okay;
}

/*  Translation  */

template<typename cbk_t>
Handle VirtualAllocationSpace::TryTranslate(vaddr_t const vaddr, cbk_t cbk, bool const tolerate)
{
    if unlikely(!tolerate && (
                (vaddr >= FractalStart && vaddr < FractalEnd     )
             || (vaddr >= LowerHalfEnd && vaddr < HigherHalfStart)))
        return HandleResult::PageMapIllegalRange;

    bool const nonLocal = (vaddr < LowerHalfEnd) && !this->IsLocal();
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

Handle VirtualAllocationSpace::GetEntry(vaddr_t const vaddr, Pml1Entry * & e, bool const tolerate)
{
    return this->TryTranslate(vaddr, [&e](Pml1Entry * pE) {
        e = pE;

        return Handle(HandleResult::Okay);
    }, tolerate);
}

/*  Mapping  */

Handle VirtualAllocationSpace::Map(vaddr_t const vaddr, paddr_t const paddr, const PageFlags flags, PageDescriptor * & pml3desc, PageDescriptor * & pml2desc, PageDescriptor * & pml1desc)
{
    if unlikely((vaddr >= FractalStart && vaddr < FractalEnd     )
             || (vaddr >= LowerHalfEnd && vaddr < HigherHalfStart))
        return HandleResult::PageMapIllegalRange;

    if unlikely((vaddr & (PageSize - 1)) != 0 || (paddr & (PageSize - 1)) != 0)
        return HandleResult::PageUnaligned;

    bool const nonLocal = (vaddr < LowerHalfEnd) && !this->IsLocal();
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

        paddr_t const newPml3 = this->Allocator->AllocatePage(pml3desc);

        if (newPml3 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml4[ind] = Pml4Entry(newPml3, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml3p, 0, PageSize);
    }

    Pml3 & pml3 = *pml3p;
    ind = GetPml3Index(vaddr);

    if unlikely(!pml3[ind].GetPresent())
    {
        assert(pml3[ind].IsNull()
            , "Absent PDPT entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        paddr_t const newPml2 = this->Allocator->AllocatePage(pml2desc);

        if (newPml2 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml3[ind] = Pml3Entry(newPml2, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml2p, 0, PageSize);
    }
    
    Pml2 & pml2 = *pml2p;
    ind = GetPml2Index(vaddr);

    if unlikely(!pml2[ind].GetPresent())
    {
        assert(pml2[ind].IsNull()
            , "Absent PD entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        paddr_t const newPml1 = this->Allocator->AllocatePage(pml1desc);

        if (newPml1 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml2[ind] = Pml2Entry(newPml1, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml1p, 0, PageSize);
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

Handle VirtualAllocationSpace::Map(vaddr_t const vaddr, paddr_t const paddr, const PageFlags flags)
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

Handle VirtualAllocationSpace::Unmap(vaddr_t const vaddr, paddr_t & paddr)
{
    return this->TryTranslate(vaddr, [&paddr](Pml1Entry * pE) {
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
        }, false);
}

/*  Flags  */

Handle VirtualAllocationSpace::GetPageFlags(vaddr_t const vaddr, PageFlags & flags)
{
    return this->TryTranslate(vaddr, [&flags](Pml1Entry * pE) {
            if likely(pE->GetPresent())
            {
                Pml1Entry const e = *pE;
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
        }, false);
}

Handle VirtualAllocationSpace::SetPageFlags(vaddr_t const vaddr, const PageFlags flags)
{
    return this->TryTranslate(vaddr, [flags](Pml1Entry * pE) {
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
        }, false);
}

/**********************************************
    VirtualAllocationSpace::Iterator struct
**********************************************/

/*  Constructor(s)  */

Handle VirtualAllocationSpace::Iterator::Create(Iterator & dst, VirtualAllocationSpace * const space, vaddr_t const vaddr)
{
    if unlikely(0 != (vaddr & (PageSize - 1)))
        return Handle(HandleResult::PageUnaligned);

    if unlikely((vaddr >= FractalStart && vaddr < FractalEnd     )
             || (vaddr >= LowerHalfEnd && vaddr < HigherHalfStart))
        return Handle(HandleResult::PageMapIllegalRange);

    dst = VirtualAllocationSpace::Iterator(space, vaddr);

    return dst.Initialize();
}

Handle VirtualAllocationSpace::Iterator::Initialize()
{
    vaddr_t const vaddr = this->VirtualAddress;
    bool const nonLocal = (vaddr < LowerHalfEnd) && !this->AllocationSpace->IsLocal();

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

    auto const work = [vaddr, nonLocal](auto const & alien, auto const & local, auto const & index) {
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
    vaddr_t const vaddr = this->VirtualAddress;
    vaddr_t const target = vaddr + (diff << 12);

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
    vaddr_t const vaddr = this->VirtualAddress;
    vaddr_t const target = vaddr + (diff << 12);

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

    ASSERT(res.IsOkayResult()
        , "Failed to create new Iterator instance: %H. (%Xp - %us pages [%Xp])"
        , res, vaddr, diff, diff << 12);
    //  Must not fail.

    return other;
}

Handle VirtualAllocationSpace::Iterator::AllocateTables(PageDescriptor * & pml3desc, PageDescriptor * & pml2desc, PageDescriptor * & pml1desc)
{
    vaddr_t const vaddr = this->VirtualAddress;
    bool const nonLocal = (vaddr < LowerHalfEnd) && !this->AllocationSpace->IsLocal();
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

        paddr_t const newPml3 = this->AllocationSpace->Allocator->AllocatePage(pml3desc);

        if (newPml3 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml4[ind] = Pml4Entry(newPml3, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml3p, 0, PageSize);
    }

    Pml3 & pml3 = *pml3p;
    ind = GetPml3Index(vaddr);

    if unlikely(!pml3[ind].GetPresent())
    {
        assert(pml3[ind].IsNull()
            , "Absent PDPT entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        paddr_t const newPml2 = this->AllocationSpace->Allocator->AllocatePage(pml2desc);

        if (newPml2 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml3[ind] = Pml3Entry(newPml2, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml2p, 0, PageSize);
    }

    Pml2 & pml2 = *pml2p;
    ind = GetPml2Index(vaddr);

    //  Yes, this one is likely to be absent.
    if likely(!pml2[ind].GetPresent())
    {
        assert(pml2[ind].IsNull()
            , "Absent PD entry (#%u2 for %Xp) is non-null!"
            , ind, vaddr);

        paddr_t const newPml1 = this->AllocationSpace->Allocator->AllocatePage(pml1desc);

        if (newPml1 == 0)
            return Handle(HandleResult::OutOfMemory);

        pml2[ind] = Pml2Entry(newPml1, true, true, true, false);
        //  Present, writable, user-accessible, executable.

        memset(pml1p, 0, PageSize);
    }

    Pml1 & pml1 = *pml1p;
    ind = GetPml1Index(vaddr);

    this->Entry = &pml1[ind];

    return Handle(HandleResult::Okay);
}
