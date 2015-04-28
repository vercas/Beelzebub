#include <memory/page_allocation.hpp>
#include <debug.hpp>
#include <math.h>

using namespace Beelzebub::Debug;

namespace Beelzebub { namespace Memory
{
    /*********************************
        PageAllocationSpace struct
    *********************************/

    /*  Constructors    */

    PageAllocationSpace::PageAllocationSpace()
    {

    }

    PageAllocationSpace::PageAllocationSpace(const uintptr_t phys_start, const uintptr_t phys_end
                                           , const size_t page_size)
    {
        //  Basics.
        this->Map = (PageDescriptor *)phys_start;
        this->PageSize = page_size;

        //  Just the ones that can be quickly calculated and are required later.
        this->PageCount = (phys_end - phys_start) / page_size;
        this->AllocablePageCount = (phys_end - phys_start) / (page_size + sizeof(PageDescriptor) + sizeof(size_t));
        this->FreePageCount = this->AllocablePageCount;

        //  Stuff we cannot touch.
        this->ControlPageCount = this->PageCount - this->FreePageCount;
        this->ReservedPageCount = this->ControlPageCount;

        //  Bytes! :D
        this->Size = phys_start - phys_end;
        this->FreeSize = this->AllocableSize = this->AllocablePageCount * page_size;
        
        //  Control structures sizes.
        this->MapSize = this->AllocablePageCount * sizeof(PageDescriptor);
        this->StackSize = this->AllocablePageCount * sizeof(size_t);

        this->ReservedSize = this->MapSize + this->StackSize;

        this->MapEnd = this->Map + this->AllocablePageCount - 1;
        this->Stack = (size_t *)(this->Map + this->AllocablePageCount);

        size_t j = 0;
        for (size_t i = 0; i < this->PageCount; ++i)
        {
            this->Map[i] = PageDescriptor(i, i < this->ControlPageCount ? PageStatus::Reserved : PageStatus::Free);
            
            if (i >= this->ControlPageCount)
                this->Stack[j++] = i;
        }

        this->StackFreeTop = this->StackCacheTop = j;

        assert(j == this->AllocablePageCount, "Number of free pages in the stack doesn't match the number of allocable pages..? %u8 vs %u8", j, this->AllocablePageCount);

        this->CurrentIndex = this->ControlPageCount;

        this->MemoryStart = phys_start;
        this->MemoryEnd = phys_end;
        this->AllocationStart = phys_start + (this->ControlPageCount * page_size);
        this->AllocationEnd = phys_end;
    }

    /*  Page manipulation  */

    Handle PageAllocationSpace::ReservePageRange(const size_t start, const size_t count)
    {
        if (start + count > this->MapSize)
            return Handle(HandleResult::ArgumentOutOfRange);

        //  TODO: Check if the pages are in use!

        for (size_t i = 0; i < count; i++)
            (this->Map + start + i)->Reserve();

        this->FreePageCount -= count;
        this->ReservedPageCount += count;

        this->FreeSize = this->FreePageCount * this->PageSize;
        this->ReservedSize = this->ReservedPageCount * this->PageSize;

        return Handle(HandleResult::Okay);
    }

    Handle PageAllocationSpace::ReserveByteRange(const uintptr_t phys_start, const size_t length)
    {
        return this->ReservePageRange((phys_start - this->MemoryStart) / this->PageSize, length / this->PageSize);
    }

    void * PageAllocationSpace::AllocatePages(const size_t count)
    {


        return nullptr;
    }

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
