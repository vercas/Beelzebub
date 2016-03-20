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

#include <memory/page_allocator.hpp>

#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::Debug;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

/****************************
    PageDescriptor struct
****************************/

/*  Reference count  */

uint32_t PageDescriptor::DecrementReferenceCount()
{
    uint32_t const ret = this->ReferenceCount--;

    //  A potentially inconvenient race condition was detected here by Griwes during
    //  a code review. Cheers! :D

    assert(ret > 0,
        "Attempting to decrement reference count of a page below 0!");

    return ret - 1;
}

/*********************************
    PageAllocationSpace struct
*********************************/

/*  Constructors    */

PageAllocationSpace::PageAllocationSpace()
    //  Just the basics.
    : MemoryStart( 0 )
    , MemoryEnd(0)
    , AllocationEnd(0)
    , PageSize(0)
    , PageCount(0)
    , Size(0)

    //  Number of pages that are allocable.
    , AllocablePageCount(0)

    //  Miscellaneous.
    , ReservedPageCount(0)
    , Map(nullptr)
    , Locker()

    //  Links
    , Next(nullptr)
    , Previous(nullptr)
{

}

PageAllocationSpace::PageAllocationSpace(const paddr_t phys_start, const paddr_t phys_end
                                       , const psize_t page_size)
    //  Just the basics.
    : MemoryStart( phys_start )
    , MemoryEnd(phys_end)
    , AllocationEnd(phys_end)
    , PageSize(page_size)
    , PageCount((phys_end - phys_start) / page_size)
    , Size(phys_end - phys_start)

    //  Number of pages that are allocable.
    , AllocablePageCount((phys_end - phys_start) / (page_size + sizeof(PageDescriptor) + sizeof(pgind_t)))

    //  Miscellaneous.
    , ReservedPageCount(0)
    , Map((PageDescriptor *)phys_start)
    , Locker()

    //  Links
    , Next(nullptr)
    , Previous(nullptr)
{
    this->FreePageCount = this->AllocablePageCount;
    this->ControlPageCount = this->PageCount - this->FreePageCount;
    this->FreeSize = this->AllocableSize = this->AllocablePageCount * page_size;
    this->ReservedSize = this->ReservedPageCount * page_size;

    this->Stack = (psize_t *)(this->Map + this->AllocablePageCount);

    this->StackFreeTop = /*this->StackCacheTop =*/ this->AllocablePageCount - 1;
    this->AllocationStart = phys_start + (this->ControlPageCount * page_size);
}

Handle PageAllocationSpace::InitializeControlStructures()
{
    /*if (debug)
    {
        msg("%nMap at %Xp; stack at %Xp%n", this->Map, this->Stack);

        TerminalCoordinates co = DebugTerminal->GetCurrentPosition();
        DebugTerminal->SetCurrentPosition(60, 4);
        DebugTerminal->WriteLine("Address         |P| NX|R/W|U/S|PWT|PCD|ACC|GLB|DRT|PAT|");
        DebugTerminal->SetCurrentPosition(co);

        for (size_t i = 0; i < this->AllocablePageCount; ++i)
        {
            if (((i >= 20000 && i <= 22500) && i % 100 == 0)
             || ((i >= 525300 && i <= 525500)  && i % 10 == 0))
            {
                paddr_t paddr = i * this->PageSize + this->AllocationStart;

                Cr3 cr3 = Cpu::GetCr3();
                Pml4 & pml4 = *cr3.GetPml4Ptr();
                Pml4Entry & e1 = pml4[{(void *)paddr}];
                Pml3 & pml3 = *e1.GetPml3Ptr();
                Pml3Entry & e2 = pml3[{(void *)paddr}];
                Pml2 & pml2 = *e2.GetPml2Ptr();
                Pml2Entry & e3 = pml2[{(void *)paddr}];

                co = DebugTerminal->GetCurrentPosition();
                DebugTerminal->SetCurrentPosition(60, 5);
                e1.PrintToTerminal(DebugTerminal);
                DebugTerminal->SetCurrentPosition(60, 6);
                e2.PrintToTerminal(DebugTerminal);
                DebugTerminal->SetCurrentPosition(60, 7);
                e3.PrintToTerminal(DebugTerminal);
                DebugTerminal->SetCurrentPosition(60, 8);
                msg("%XP - page #%us ", paddr, i);
                DebugTerminal->SetCurrentPosition(co);

                breakpoint();
            }

            this->Map[i] = PageDescriptor(this->Stack[i] = i, PageDescriptorStatus::Free);
        }
    }
    else //*/

    for (size_t i = 0; i < this->AllocablePageCount; ++i)
        //this->Map[i] = PageDescriptor(this->Stack[i] = i, PageDescriptorStatus::Free);
        new (this->Map + i) PageDescriptor(this->Stack[i] = i, PageDescriptorStatus::Free);

    return HandleResult::Okay;
}

/*  Page manipulation  */

Handle PageAllocationSpace::ReservePageRange(const pgind_t start, const psize_t count, const PageReservationOptions options)
{
    if unlikely(start + count > this->AllocablePageCount)
        return HandleResult::PagesOutOfAllocatorRange;

    bool const inclInUse    = (0 != (options & PageReservationOptions::IncludeInUse  ));
    bool const ignrReserved = (0 != (options & PageReservationOptions::IgnoreReserved));

    PageDescriptor * const map = this->Map + start;

    for (pgind_t i = 0; i < count; ++i)
    {
        PageDescriptor * const page = map + i;
        PageDescriptorStatus const status = page->Status;

        /*msg("Reserving page #%us (%Xp @%us: %s).%n", start + i
            , this->MemoryStart + (start + i) * this->PageSize
            , i, page->GetStatusString());//*/

        if likely(status == PageDescriptorStatus::Free)
        {
            /*msg("  SI: %us; SFT: %us; SCT: %us.%n"
                , page->StackIndex
                , this->StackFreeTop
                , this->StackCacheTop);//*/

            withLock (this->Locker)
            {
                this->PopPage(start + i);

                page->Reserve();
            }
        }
        else if (status == PageDescriptorStatus::InUse)
        {
            if (inclInUse)
                withLock (this->Locker)
                    page->Reserve();
            else
                return HandleResult::PageInUse;

            //  Yes, locking is enough. It will reserve and unlock later.
        }
        else if (ignrReserved)
            continue;
        else
            return HandleResult::PageReserved;
    }

    this->FreePageCount -= count;
    this->ReservedPageCount += count;

    this->FreeSize = this->FreePageCount * this->PageSize;
    this->ReservedSize = this->ReservedPageCount * this->PageSize;

    return HandleResult::Okay;
}

Handle PageAllocationSpace::FreePageRange(const pgind_t start, const psize_t count)
{
    if unlikely(start + count > this->AllocablePageCount)
        return HandleResult::PagesOutOfAllocatorRange;

    PageDescriptor * const map = this->Map + start;

    for (pgind_t i = 0; i < count; ++i)
    {
        PageDescriptor * const page = map + i;
        const PageDescriptorStatus status = page->Status;

        if likely(status == PageDescriptorStatus::InUse)
            withLock (this->Locker)
            {
                this->Stack[page->StackIndex = ++this->StackFreeTop] = start + i;

                page->Free();
            }
        else
        {
            //  Reserved pages won't be freed by this function.
            //  Free pages are already free.

            if (status == PageDescriptorStatus::Reserved)
                return HandleResult::PageReserved;
            else
                return HandleResult::PageFree;

            //  The proper error must be returned!
        }
    }

    return HandleResult::Okay;
}

paddr_t PageAllocationSpace::AllocatePage(PageDescriptor * & desc)
{
    if likely(this->FreePageCount != 0)
    {
        pgind_t i;

        withLock (this->Locker)
        {
            i = this->Stack[this->StackFreeTop--];

            (desc = this->Map + i)->Use();
            //  Mark the page as used.
            //  And, of course, return the descriptor.

            --this->FreePageCount;
            this->FreeSize -= this->PageSize;
            //  Change the info accordingly.
        }

        return this->AllocationStart + i * this->PageSize;
    }

    desc = nullptr;

    return nullpaddr;
}

paddr_t PageAllocationSpace::AllocatePages(const psize_t count)
{
    if (count == 1)
    {
        PageDescriptor * desc;

        return this->AllocatePage(desc);
    }
    else if (count == 0)
        return nullpaddr;

    //  TODO: Implement this in the most noobish way imaginable.

    return nullpaddr;
}

/*  Utilitary Methods  */

Handle PageAllocationSpace::PopPage(const pgind_t ind)
{
    if unlikely(ind >= this->AllocablePageCount)
        return HandleResult::PagesOutOfAllocatorRange;

    PageDescriptor * const page = this->Map + ind;

    if (page->Status == PageDescriptorStatus::Free)
    {
        PageDescriptor * freeTop = this->Map + this->Stack[this->StackFreeTop];

        //  Popping off el stacko.
        this->Stack[page->StackIndex] = this->Stack[this->StackFreeTop--];
        freeTop->StackIndex = page->StackIndex;

        --this->FreePageCount;
        this->FreeSize -= this->PageSize;
        //  Change the info accordingly.

        return HandleResult::Okay;
    }

    return HandleResult::PageNotStacked;
}

/*  Debug  */

#ifdef __BEELZEBUB__DEBUG
TerminalWriteResult PageAllocationSpace::PrintStackToTerminal(TerminalBase * const term, const bool details)
{
    TerminalWriteResult tret;
    uint32_t cnt;

    if (details)
    {
        TERMTRY0(term->WriteFormat("--|T|M|  Stack  Index  |   Page Index   | ... PR: %Xp-%Xp; STK: %Xp--%n", this->AllocationStart, this->AllocationEnd, this->Stack), tret);

        for (size_t i = 0; i <= this->StackFreeTop; ++i)
        {
            TERMTRY1(term->Write("  |F|"), tret, cnt);
            TERMTRY1(term->Write((i == this->Stack[i]) ? ' ' : 'X'), tret, cnt);
            TERMTRY1(term->Write('|'), tret, cnt);
            TERMTRY1(term->WriteHex64((uint64_t)i), tret, cnt);
            TERMTRY1(term->Write('|'), tret, cnt);
            TERMTRY1(term->WriteHex64((uint64_t)(this->Stack[i])), tret, cnt);
            TERMTRY1(term->WriteLine(""), tret, cnt);
        }
    }
    else
    {
        TERMTRY0(term->WriteFormat("--|T|M|  Stack  Index  |   Page Index   | ... PR: %Xp-%Xp; STK: %Xp--%n", this->MemoryStart, this->MemoryEnd, this->Stack), tret);

        for (size_t i = 0; i <= this->StackFreeTop; ++i)
        {
            TERMTRY1(term->Write("  |F|"), tret, cnt);
            TERMTRY1(term->Write((i == this->Stack[i]) ? ' ' : 'X'), tret, cnt);
            TERMTRY1(term->Write('|'), tret, cnt);
            TERMTRY1(term->WriteHex64((uint64_t)i), tret, cnt);
            TERMTRY1(term->Write('|'), tret, cnt);
            TERMTRY1(term->WriteHex64((uint64_t)(this->Stack[i])), tret, cnt);
            TERMTRY1(term->WriteLine(""), tret, cnt);
        }
    }

    return tret;
}
#endif

/***************************
    PageAllocator struct
***************************/

/*  Constructors  */

PageAllocator::PageAllocator()
    : ChainLock()
    , FirstSpace(nullptr)
    , LastSpace(nullptr)
{
    
}

PageAllocator::PageAllocator(PageAllocationSpace * const first)
    : ChainLock()
    , FirstSpace(first)
    , LastSpace(first)
{
    assert(first != nullptr
        , "Attempted to construct a page allocator with a null first allocation space.");
    assert(first->Previous == nullptr
        , "Attempted to construct a page allocator with a first allocation space that seems to be linked to a previous one.");

    while (this->LastSpace->Next != nullptr)
    {
        //  While the last allocation space links to a 'next one'...
        
        assert(this->LastSpace == this->LastSpace->Next->Previous
            , "Linking error in allocation spaces!");
        //  Assert proper linkage between the allocation spaces.

        this->LastSpace = this->LastSpace->Next;
        //  The next will become the last.
    }
}

/*  Page Manipulation  */

Handle PageAllocator::ReserveByteRange(const paddr_t phys_start, const psize_t length, const PageReservationOptions options)
{
    Handle res;
    PageAllocationSpace * space = this->FirstSpace;

    while (space != nullptr)
    {
        res = space->ReserveByteRange(phys_start, length, options);

        if (!res.IsResult(HandleResult::PagesOutOfAllocatorRange))
            return res;

        space = space->Next;
    }

    return HandleResult::PagesOutOfAllocatorRange;
}

Handle PageAllocator::FreeByteRange(const paddr_t phys_start, const psize_t length)
{
    Handle res;
    PageAllocationSpace * space = this->FirstSpace;

    while (space != nullptr)
    {
        res = space->FreeByteRange(phys_start, length);

        if (!res.IsResult(HandleResult::PagesOutOfAllocatorRange))
            return res;

        space = space->Next;
    }

    return HandleResult::PagesOutOfAllocatorRange;
}

Handle PageAllocator::FreePageAtAddress(const paddr_t phys_addr)
{
    Handle res;
    PageAllocationSpace * space = this->FirstSpace;

    while (space != nullptr)
    {
        res = space->FreePageAtAddress(phys_addr);

        if (!res.IsResult(HandleResult::PagesOutOfAllocatorRange))
            return res;

        space = space->Next;
    }

    return HandleResult::PagesOutOfAllocatorRange;
}

paddr_t PageAllocator::AllocatePage(const PageAllocationOptions options, PageDescriptor * & desc)
{
    paddr_t ret = nullpaddr;
    PageAllocationSpace * space;

    if (0 != (options & PageAllocationOptions::ThirtyTwoBit))
    {
        space = this->LastSpace;

        while (space != nullptr)
        {
            if ((uint64_t)space->GetAllocationEnd() <= (1ULL << 32))
            {
                //  The condition checks that the allocation space ends
                //  at a 32-bit address. (all the other addresses are less,
                //  thus have to be 32-bit if the end is)

                ret = space->AllocatePage(desc);

                if (ret != nullpaddr)
                    return ret;
            }

            space = space->Previous;
        }
    }
    else
    {
        space = this->FirstSpace;

        while (space != nullptr)
        {
            ret = space->AllocatePage(desc);

            if (ret != nullpaddr)
                return ret;

            space = space->Next;
        }
    }

    desc = nullptr;

    return nullpaddr;
}

paddr_t PageAllocator::AllocatePages(const psize_t count, const PageAllocationOptions options)
{
    paddr_t ret = nullpaddr;
    PageAllocationSpace * space;

    if (0 != (options & PageAllocationOptions::ThirtyTwoBit))
    {
        space = this->LastSpace;

        while (space != nullptr)
        {
            if ((uint64_t)space->GetAllocationEnd() <= (1ULL << 32))
            {
                //  The condition checks that the allocation space ends
                //  at a 32-bit address. (all the other addresses are less,
                //  thus have to be 32-bit if the end is)

                ret = space->AllocatePages(count);

                if (ret != nullpaddr)
                    return ret;
            }

            space = space->Previous;
        }
    }
    else
    {
        space = this->FirstSpace;

        while (space != nullptr)
        {
            ret = space->AllocatePages(count);

            if (ret != nullpaddr)
                return ret;

            space = space->Next;
        }
    }

    return nullpaddr;
}

PageAllocationSpace * PageAllocator::GetSpaceContainingAddress(const paddr_t address)
{
    PageAllocationSpace * space = this->FirstSpace;

    while (space != nullptr)
    {
        if (space->ContainsRange(address, 1))
            return space;

        space = space->Next;
    }

    return nullptr;
}

bool PageAllocator::ContainsRange(const paddr_t phys_start, const psize_t length)
{
    const PageAllocationSpace * space = this->FirstSpace;

    while (space != nullptr)
    {
        if (space->ContainsRange(phys_start, length))
            return true;

        space = space->Next;
    }

    return false;
}

bool PageAllocator::TryGetPageDescriptor(const paddr_t paddr, PageDescriptor * & res)
{
    PageAllocationSpace * space = this->FirstSpace;

    while (space != nullptr)
    {
        if (space->TryGetPageDescriptor(paddr, res))
            return true;

        space = space->Next;
    }

    return false;
}

/*  Space Chaining  */

void PageAllocator::PreppendAllocationSpace(PageAllocationSpace * const space)
{
    withLock (this->ChainLock)
    {
        this->FirstSpace->Previous = space;
        space->Next = this->FirstSpace;
        this->FirstSpace = space;
    }
}

void PageAllocator::AppendAllocationSpace(PageAllocationSpace * const space)
{
    withLock (this->ChainLock)
    {
        this->LastSpace->Next = space;
        space->Previous = this->LastSpace;
        this->LastSpace = space;
    }
}

void PageAllocator::RemapLinks(const vaddr_t oldAddr, const vaddr_t newAddr)
{
    withLock (this->ChainLock)
    {
        const vaddr_t firstAddr   = (vaddr_t)this->FirstSpace;
        const vaddr_t lastAddr    = (vaddr_t)this->LastSpace;
        const vaddr_t oldVaddr    = oldAddr;
        const vaddr_t newVaddr    = newAddr;
        const vaddr_t oldVaddrEnd = oldVaddr + PageSize;

        if (firstAddr > 0 && firstAddr >= oldVaddr && firstAddr < oldVaddrEnd)
            this->FirstSpace = (PageAllocationSpace *)((firstAddr - oldVaddr) + newVaddr);
        if (lastAddr > 0 && lastAddr >= oldVaddr && lastAddr < oldVaddrEnd)
            this->LastSpace = (PageAllocationSpace *)((lastAddr - oldVaddr) + newVaddr);

        //  Now makin' sure all the pointers are aligned.

        PageAllocationSpace * cur = this->LastSpace;

        while (cur != nullptr)
        {
            const vaddr_t nextAddr = (vaddr_t)cur->Next;
            const vaddr_t prevAddr = (vaddr_t)cur->Previous;

            if (nextAddr > 0 && nextAddr >= oldVaddr && nextAddr < oldVaddrEnd)
                cur->Next = (PageAllocationSpace *)((nextAddr - oldVaddr) + newVaddr);
            if (prevAddr > 0 && prevAddr >= oldVaddr && prevAddr < oldVaddrEnd)
                cur->Previous = (PageAllocationSpace *)((prevAddr - oldVaddr) + newVaddr);

            cur = cur->Previous;
        }
    }
}
