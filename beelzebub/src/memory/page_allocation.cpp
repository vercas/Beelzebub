#include <memory/page_allocation.hpp>

namespace Beelzebub { namespace Memory
{
	/*********************************
		PageAllocationSpace struct
	*********************************/

	/*	Constructors	*/

	PageAllocationSpace::PageAllocationSpace()
	{

	}

	PageAllocationSpace::PageAllocationSpace(const uintptr_t phys_start, const uintptr_t phys_end
								           , const size_t page_size)
	{
		this->Map = (PageDescriptor *)phys_start;
		this->PageSize = page_size;

		this->PageCount = (phys_end - phys_start) / page_size;
		this->AllocablePageCount = (phys_end - phys_start) / (page_size + sizeof(PageDescriptor));
		this->FreePageCount = this->AllocablePageCount;
		this->MapPageCount = this->PageCount - this->FreePageCount;
		this->ReservedPageCount = this->MapPageCount;

		this->Size = phys_start - phys_end;
		this->FreeSize = this->AllocableSize = this->AllocablePageCount * page_size;
		this->ReservedSize = this->MapSize = this->MapPageCount * page_size;

		for (size_t i = 0; i < this->FreePageCount; ++i)
			this->Map[i] = PageDescriptor(i < this->MapPageCount ? PageStatus::Reserved : PageStatus::Free);

		this->MapEnd = this->Map + this->MapPageCount - 1;

		this->CurrentIndex = this->MapPageCount;

		this->MemoryStart = phys_start;
		this->MemoryEnd = phys_end;
		this->AllocationStart = phys_start + (this->MapPageCount * page_size);
		this->AllocationEnd = phys_end;
	}

	/*  Page manipulation  */

	Result PageAllocationSpace::ReservePageRange(const size_t start, const size_t count)
	{
		if (start + count > this->MapSize)
			return Result::ArgumentOutOfRange;

		//  TODO: Check if the pages are in use!

		for (size_t i = 0; i < count; i++)
			(this->Map + start + i)->Reserve();

		this->FreePageCount -= count;
		this->ReservedPageCount += count;

		this->FreeSize = this->FreePageCount * this->PageSize;
		this->ReservedSize = this->ReservedPageCount * this->PageSize;

		return Result::Okay;
	}

	Result PageAllocationSpace::ReserveByteRange(const uintptr_t phys_start, const size_t length)
	{
		return this->ReservePageRange((phys_start - this->MemoryStart) / this->PageSize, length / this->PageSize);
	}

	/****************************
		PageDescriptor struct
	****************************/

	/*	Accesses	*/

	u8 PageDescriptor::IncrementAccesses()
	{
		if ((val | 3) == 0xFF)
			val = (val & 0x3) | 0x4;
		else
			val += 0x4;

		//	If the top 6 bits are all 1, then the count is reset to 1.
		//	(2 LSBs preserved)
		//	Otherwise, 4 is added, incrementing the 3rd LSB.

		return val >> 2;
	}
}}
