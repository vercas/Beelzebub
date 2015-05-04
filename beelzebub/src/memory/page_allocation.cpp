#include <memory/page_allocation.hpp>
#include <debug.hpp>
#include <math.h>

using namespace Beelzebub::Terminals;
using namespace Beelzebub::Debug;

namespace Beelzebub { namespace Memory
{
    /*********************************
        PageAllocationSpace struct
    *********************************/

    /*  Constructors    */

    PageAllocationSpace::PageAllocationSpace()
        : MemoryStart( 0 )
        , MemoryEnd(0)
        , PageSize(0)
        , Size(0)
    {

    }

    PageAllocationSpace::PageAllocationSpace(const paddr_t phys_start, const paddr_t phys_end
                                           , const psize_t page_size)
        : MemoryStart( phys_start )
        , MemoryEnd(phys_end)
        , PageSize(page_size)
        , Size(phys_end - phys_start)
    {
        //  Basics.
        this->Map = (PageDescriptor *)phys_start;

        //  Just the ones that can be quickly calculated and are required later.
        this->PageCount = (phys_end - phys_start) / page_size;
        this->AllocablePageCount = (phys_end - phys_start) / (page_size + sizeof(PageDescriptor) + sizeof(pgind_t));
        this->FreePageCount = this->AllocablePageCount;

        //  Stuff we cannot touch.
        this->ControlPageCount = this->PageCount - this->FreePageCount;
        this->ReservedPageCount = this->ControlPageCount;

        //  Bytes! :D
        this->FreeSize = this->AllocableSize = this->AllocablePageCount * page_size;

        //  Control structures sizes.
        this->MapSize = this->AllocablePageCount * sizeof(PageDescriptor);
        this->StackSize = this->AllocablePageCount * sizeof(pgind_t);

        //  Technically, the whole pages are reserved.
        this->ReservedSize = this->ReservedPageCount * page_size;

        this->Stack = (psize_t *)(this->Map + this->AllocablePageCount);

        /*msg("--Creating page allocator - M:%Xp; S:%Xp; PC:%us; MS:%us; SS:%us;%n"
            , this->Map, this->Stack
            , this->PageCount, this->MapSize, this->StackSize);
        msg("-- AS:%us; FS:%us; RS:%us;%n"
            , this->Size, this->FreeSize, this->ReservedSize);
        msg("-- S-M:%u8; Diff:%u8%n"
            , (uint64_t)this->Stack - (uint64_t)this->Map
            , (size_t)this->Stack - (size_t)this->Map - this->MapSize);

        msg("--|T|   Page Index   |  A Page Index  | A. Stack Index |  Desc Pointer  |%n");//*/

        for (size_t i = 0; i < this->AllocablePageCount; ++i)
        {
            this->Map[i] = PageDescriptor(this->Stack[i] = i, PageStatus::Free);

            /*msg("  |F|%X8|%X8|%X8|%X8|%n", (uint64_t)i
                                         , (uint64_t)this->Stack[i]
                                         , (uint64_t)this->Map[i].StackIndex
                                         , (uint64_t)(this->Map + i));//*/
        }

        this->StackFreeTop = this->StackCacheTop = this->AllocablePageCount - 1;

        this->AllocationStart = phys_start + (this->ControlPageCount * page_size);
        this->AllocationEnd = phys_end;

        /*assert(j == this->AllocablePageCount
            , "Number of free pages in the stack doesn't match the number of allocable pages..? %u8 vs %u8"
            , j, this->AllocablePageCount);//*/

        assert(this->ControlPageCount + this->AllocablePageCount == this->PageCount
            , "Page count mismatch..? %u8 + %u8 != %u8"
            , this->ControlPageCount, this->AllocablePageCount
            , this->PageCount);

        /*msg(">>&LOCK|%Xp|&LOCK<<%n%n", &this->Locker);

        msg(">>LOCK|%Xs|LOCK<<%n", *((vsize_t *)(&this->Locker)));
        msg(">>LOCK|%Xs|LOCK<<%n", this->Locker.GetValue());
        msg(">>LOCK|%Xs|LOCK<<%n", (&this->Locker)->GetValue());

        this->Unlock();

        msg(">>LOCK|%Xs|LOCK<<%n", *((vsize_t *)(&this->Locker)));
        msg(">>LOCK|%Xs|LOCK<<%n", this->Locker.GetValue());
        msg(">>LOCK|%Xs|LOCK<<%n", (&this->Locker)->GetValue());//*/
    }

    /*  Page manipulation  */

    Handle PageAllocationSpace::ReservePageRange(const pgind_t start, const psize_t count, const bool onlyFree)
    {
        if (start + count > this->MapSize)
            return Handle(HandleResult::ArgumentOutOfRange);

        PageDescriptor * map = this->Map + start - this->ControlPageCount;

        for (pgind_t i = 0; i < count; ++i)
        {
            this->Lock();

            PageDescriptor * page = map + i;

            /*msg("Reserving page #%us (%Xp @%us: %s).%n", start + i
                , this->MemoryStart + (start + i) * this->PageSize
                , i, page->GetStatusString());//*/

            if (page->Status == PageStatus::Free)
            {
                /*msg("  SI: %us; SFT: %us; SCT: %us.%n"
                    , page->StackIndex
                    , this->StackFreeTop
                    , this->StackCacheTop);//*/

                //  Popping off el stacko.
                this->Stack[page->StackIndex] = this->Stack[this->StackFreeTop--];

                if (this->StackCacheTop != this->StackFreeTop + 1)
                    this->Stack[this->StackFreeTop + 1] = this->Stack[this->StackCacheTop--];

                --this->FreePageCount;
                this->FreeSize -= this->PageSize;
                //  Change the info accordingly.
            }
            else if (!onlyFree && page->Status == PageStatus::Caching)
            {
                /*msg("  SI: %us; SCT: %us.%n"
                    , page->StackIndex
                    , this->StackCacheTop);//*/

                //  TODO: Perhaps notify of this event?

                //  Popping off el stacko.
                this->Stack[page->StackIndex] = this->Stack[this->StackCacheTop--];
            }
            else
            {
                this->Unlock();

                if (onlyFree)
                    return Handle(HandleResult::PagesNotFree);
                else
                    return Handle(HandleResult::PagesNotFreeOrCaching);
                //  The proper error must be returned!
            }

            page->Reserve();

            this->Unlock();
        }

        this->FreePageCount -= count;
        this->ReservedPageCount += count;

        this->FreeSize = this->FreePageCount * this->PageSize;
        this->ReservedSize = this->ReservedPageCount * this->PageSize;

        return Handle(HandleResult::Okay);
    }

    Handle PageAllocationSpace::FreePageRange(const pgind_t start, const psize_t count)
    {
        if (start + count > this->MapSize)
            return Handle(HandleResult::ArgumentOutOfRange);

        PageDescriptor * map = this->Map + start - this->ControlPageCount;

        for (pgind_t i = 0; i < count; ++i)
        {
            this->Lock();

            PageDescriptor * page = map + i;
            PageStatus status = page->Status;

            if (status == PageStatus::InUse)
            {
                if (this->StackCacheTop != this->StackFreeTop)
                    this->Stack[++this->StackCacheTop] = this->Stack[this->StackFreeTop];

                this->Stack[page->StackIndex = ++this->StackFreeTop] = start + i;
            }
            else if (status == PageStatus::Caching)
            {
                //  TODO: Perhaps notify of this event?

                this->Stack[page->StackIndex] = this->Stack[++this->StackFreeTop];
                //  Move the bottom free page to the position of the current page.

                this->Stack[page->StackIndex = this->StackFreeTop] = start + i;
            }
            else
            {
                this->Unlock();

                //  Reserved pages won't be freed by this function.
                //  Free pages are already free.

                if (status == PageStatus::Reserved)
                    return Handle(HandleResult::PageReserved);
                else
                    return Handle(HandleResult::PageFree);

                //  The proper error must be returned!
            }

            page->Free();

            this->Unlock();
        }

        return Handle(HandleResult::Okay);
    }

    paddr_t PageAllocationSpace::AllocatePage()
    {
        if (this->FreePageCount != 0)
        {
            this->Lock();

            pgind_t i = this->Stack[this->StackFreeTop--];

            if (this->StackCacheTop != this->StackFreeTop + 1)
                this->Stack[this->StackFreeTop + 1] = this->Stack[this->StackCacheTop--];

            (this->Map + i - this->ControlPageCount)->Use();
            //  Mark the page as used.

            --this->FreePageCount;
            this->FreeSize -= this->PageSize;
            //  Change the info accordingly.

            //  UNLOCK

            return this->MemoryStart + i * this->PageSize;
        }

        return nullptr;
    }

    paddr_t PageAllocationSpace::AllocatePages(const psize_t count)
    {
        if (count == 1)
            return this->AllocatePage();

        //  TODO: Implement this in the most noobish way imaginable.

        return nullptr;
    }

    /*  Debug  */

#ifdef __BEELZEBUB__DEBUG
    TerminalWriteResult PageAllocationSpace::PrintStackToTerminal(TerminalBase * const term, const bool details)
    {
        TerminalWriteResult tret;
        uint32_t cnt;
    
        if (details)
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

            if (this->StackCacheTop != this->StackFreeTop)
            {
                TERMTRY1(term->WriteLine("  NEXT ARE CACHING PAGES"), tret, cnt);

                for (size_t i = this->StackFreeTop + 1; i <= this->StackCacheTop; ++i)
                {
                    TERMTRY1(term->Write("  |C|"), tret, cnt);
                    TERMTRY1(term->Write((i == this->Stack[i]) ? ' ' : 'X'), tret, cnt);
                    TERMTRY1(term->Write('|'), tret, cnt);
                    TERMTRY1(term->WriteHex64((uint64_t)i), tret, cnt);
                    TERMTRY1(term->Write('|'), tret, cnt);
                    TERMTRY1(term->WriteHex64((uint64_t)(this->Stack[i])), tret, cnt);
                    TERMTRY1(term->WriteLine(""), tret, cnt);
                }
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

            if (this->StackCacheTop != this->StackFreeTop)
            {
                TERMTRY1(term->WriteLine("  NEXT ARE CACHING PAGES"), tret, cnt);

                for (size_t i = this->StackFreeTop + 1; i <= this->StackCacheTop; ++i)
                {
                    TERMTRY1(term->Write("  |C|"), tret, cnt);
                    TERMTRY1(term->Write((i == this->Stack[i]) ? ' ' : 'X'), tret, cnt);
                    TERMTRY1(term->Write('|'), tret, cnt);
                    TERMTRY1(term->WriteHex64((uint64_t)i), tret, cnt);
                    TERMTRY1(term->Write('|'), tret, cnt);
                    TERMTRY1(term->WriteHex64((uint64_t)(this->Stack[i])), tret, cnt);
                    TERMTRY1(term->WriteLine(""), tret, cnt);
                }
            }
        }

        return tret;
    }
#endif

    /****************************
        PageDescriptor struct
    ****************************/

    /*  Accesses    */

    /*u8 PageDescriptor::IncrementAccesses()
    {
        if ((val | 3) == 0xFF)
            val = (val & 0x3) | 0x4;
        else
            val += 0x4;

        //  If the top 6 bits are all 1, then the count is reset to 1.
        //  (2 LSBs preserved)
        //  Otherwise, 4 is added, incrementing the 3rd LSB.

        return val >> 2;
    }//*/
}}
