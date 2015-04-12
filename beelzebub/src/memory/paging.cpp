#include <memory/paging.hpp>
#include <screen.h>

namespace Beelzebub { namespace Memory { namespace Paging {

	PageDescriptor * Map, * MapEnd;
	size_t MapSize, CurrentIndex;

	void InitializePageAllocator(uintptr_t start, uintptr_t end)
	{
		Map = (PageDescriptor *) start;

		size_t totalPages = (end - start) / PageSize;
		size_t freePages = (end - start) / (PageSize + sizeof(PageDescriptor));
		size_t mapPages = totalPages - freePages;

		for (size_t i = 0; i < freePages; ++i)
			Map[i] = PageDescriptor(i < mapPages ? PageStatus::Reserved : PageStatus::Free);

		MapEnd = Map + mapPages	- 1;
		MapSize = mapPages;

		CurrentIndex = mapPages;

		screen_write_hex32((u32)totalPages, 1, 24);
		screen_write_hex32((u32)freePages, 21, 24);
		screen_write_hex32((u32)mapPages, 41, 24);
	}

	Result ReservePageRange(size_t const start, size_t const count)
	{
		if (start + count >= MapSize)
			return Result::ArgumentOutOfRange;

		for (size_t i = 0; i < count; i++)
			Map[start + i].Reserve();

		return Result::Okay;
	}

	Result ReservePhysicalRegion(size_t const start, size_t const length)
	{
		return ReservePageRange(start / PageSize, length / PageSize);
	}

	/****************************
		PageDescriptor struct
	****************************/

	/*	Constructors	*/

	PageDescriptor::PageDescriptor()
		: val(0)
	{
		
	}

	PageDescriptor::PageDescriptor(const PageStatus st)
		: val((u8)st)
	{
		
	}

	/*	Status	*/

	PageStatus PageDescriptor::GetStatus() const
	{
		return (PageStatus)(val & 0x3);
		//	The two least significant bits.
	}

	PageStatus PageDescriptor::SetStatus(PageStatus const st)
	{
		val = (val & 0xFC) | ((u8)st & 0x3);
		//	Yep, protection included. Mainly against myself, lawl.

		return st;
	}

	/*	Accesses	*/

	u8 PageDescriptor::GetAccesses() const 
	{
		return val >> 2;
		//	Top 6 bits.
	}

	void PageDescriptor::ResetAccesses()
	{
		val &= 0x3;
		//	Only preserve the two LSBs.
	}

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

	/*	Overalls	*/

	void PageDescriptor::Free()
	{
		val = 0;
	}

	void PageDescriptor::Use()
	{
		val = (u8)PageStatus::InUse;
	}

	void PageDescriptor::Reserve()
	{
		val = 0xFC | (u8)PageStatus::Reserved;
	}
}}}
