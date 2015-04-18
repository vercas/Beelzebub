#pragma once

#include <handles.h>

namespace Beelzebub { namespace Memory
{
	/**
	 * Represents possible statuses of a memory page.
	 */
	enum class PageStatus : u8
	{
		Free = 0,
		Caching = 1,
		InUse = 2,
		Reserved = 3,
	};

	/**
	 * Describes a page of memory.
	 */
	struct PageDescriptor
	{
		u8 val;

		/*	Constructors	*/

		__bland __forceinline PageDescriptor()
			: val(0)
		{

		}

		__bland __forceinline PageDescriptor(const PageStatus st)
			: val((u8)st)
		{

		}

		/*	Status	*/

		__bland __forceinline PageStatus GetStatus() const
		{
			return (PageStatus)(this->val & 0x3);
			//	The two least significant bits.
		}

		__bland __forceinline PageStatus SetStatus(PageStatus const st)
		{
			this->val = (this->val & 0xFC) | ((u8)st & 0x3);
			//	Yep, protection included. Mainly against myself, lawl.

			return st;
		}

		/*	Accesses	*/

		__bland __forceinline u8 GetAccesses() const 
		{
			return this->val >> 2;
			//	Top 6 bits.
		}

		__bland __forceinline void ResetAccesses()
		{
			val &= 0x3;
			//	Only preserve the two LSBs.
		}

		__bland u8 IncrementAccesses();

		/*	Overalls	*/

		__bland __forceinline void Free()
		{
			val = 0;
		}

		__bland __forceinline void Use()
		{
			val = (u8)PageStatus::InUse;
		}

		__bland __forceinline void Reserve()
		{
			val = 0xFC | (u8)PageStatus::Reserved;
		}

	} __attribute__((packed));

	/**
	 * Describes a region of memory in which pages can be allocated.
	 */
	class PageAllocationSpace
	{
		/*  TODO:
		 *  - Mutual exclusion: - Maybe over the whole allocator, but this would shuck.
		 *                      - Maybe over ranges over every N pages, but this may complicate
		 *                        the code.
		 *  - Maybe take care of page colouring?
		 */

		/*  Proeprties  */

#define PROP(type, name)                               \
	private:                                           \
		type name;                                     \
	public:                                            \
		__bland __forceinline type MCATS2(Get, name)() \
		{                                              \
			return this->name;                         \
		}

		PROP(uintptr_t, MemoryStart)        //  Start of the allocation space.
		PROP(uintptr_t, MemoryEnd)          //  End of the allocation space.
		PROP(uintptr_t, AllocationStart)    //  Start of space which can be freely allocated.
		PROP(uintptr_t, AllocationEnd)      //  End of space which can be freely allocated.

		PROP(size_t, PageSize)              //  Size of a memory page.

		PROP(size_t, PageCount)             //  Total number of pages in the allocation space.
		PROP(size_t, Size)                  //  Total number of bytes in the allocation space.
		PROP(size_t, AllocablePageCount)    //  Total number of pages which can be allocated.
		PROP(size_t, AllocableSize)         //  Total number of bytes which can be allocated.
		PROP(size_t, MapPageCount)          //  Number of pages used for mapping the space.
		PROP(size_t, MapSize)               //  Number of bytes in pages used for mapping.

		PROP(size_t, FreePageCount)         //  Number of unallocated pages.
		PROP(size_t, FreeSize)              //  Number of bytes in unallocated pages.
		PROP(size_t, ReservedPageCount)     //  Number of reserved pages.
		PROP(size_t, ReservedSize)          //  Number of bytes in reserved pages.

	public:

		/*	Constructors	*/

		__bland PageAllocationSpace();
		__bland PageAllocationSpace(const uintptr_t phys_start, const uintptr_t phys_end
							      , const size_t page_size);

		/*  Page manipulation  */

		__bland Result ReservePageRange(const size_t start, const size_t count);
		__bland Result ReserveByteRange(const uintptr_t phys_start, const size_t length);

	private:

		/*  Fields  */

		PageDescriptor * Map, * MapEnd;
		//  Pointers to the allocation map within the space.
		size_t CurrentIndex;
		//  Used for round-robin checkin'.

		//  Current size: 144 bytes. :(

	public:

		//PageAllocationSpace * Next;

	} __attribute__((packed));
}}
