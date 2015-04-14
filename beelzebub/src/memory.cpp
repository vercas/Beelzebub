#include <memory.h>
#include <math.h>
#include <arc/memory/paging.hpp>

#include <arc/screen.h>

using namespace Beelzebub::Memory::Paging;

void InitializeMemory(jg_info_mmap_t * map, int cnt, uintptr_t freeStart)
{
	//	First step is aligning the memory map.
	//	Also, yes, I could've used bits 'n powers of two here.
	//	But I'm hoping to future-proof the code a bit, in case
	//	I would ever target a platform whose page size is not
	//	A power of two.
	//	Moreover, this code doesn't need to be lightning-fast. :L

	uintptr_t start = RoundUp(freeStart, PageSize),
	    	  end = 0;
	jg_info_mmap_t * firstMap = nullptr,
	               *  lastMap = nullptr;

	for (int i = 0; i < cnt; i++)
	{
		jg_info_mmap_t * m = map + i;
		//	Current map.

		if ((m->address + m->length) <= freeStart || !m->available)
			continue;

		if (firstMap == nullptr)
			firstMap = m;

		uintptr_t addressMisalignment = RoundUpDiff(m->address, PageSize);
		//	The address is rounded up to the closest page;

		m->address += addressMisalignment;
		m->length -= addressMisalignment;

		m->length -= m->length % PageSize;
		//	The length is rounded down.

		uintptr_t mEnd = m->address + m->length;

		if (mEnd > end)
		{
			end = mEnd;
			lastMap = m;
		}
	}

	InitializePageAllocator(start, end);

	for (jg_info_mmap_t * m = firstMap; m <= lastMap; m++)
	{
		if (!m->available)
			ReservePhysicalRegion(m->address, m->length);
	}
}
