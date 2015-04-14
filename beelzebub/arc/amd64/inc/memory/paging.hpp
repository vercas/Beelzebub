#pragma once

#include <handles.h>

namespace Beelzebub { namespace Memory { namespace Paging {
	const size_t PageSize = 4 * 1024;

	void InitializePageAllocator(uintptr_t const start, uintptr_t const end) __bland;

	Result ReservePageRange(size_t const start, size_t const count) __bland;
	Result ReservePhysicalRegion(size_t const start, size_t const length) __bland;
	
	//void Initialize(void * start, )
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
		PageDescriptor() __bland;
		PageDescriptor(const PageStatus st) __bland;

		/*	Status	*/
		PageStatus GetStatus() const __bland;
		PageStatus SetStatus(PageStatus const st) __bland;

		/*	Accesses	*/
		u8 GetAccesses() const __bland;
		void ResetAccesses() __bland;
		u8 IncrementAccesses() __bland;

		/*	Overalls	*/
		void Free() __bland;
		void Use() __bland;
		void Reserve() __bland;
	} __attribute__((packed));

	/**
	 * Describes a region of memory in which pages can be allocated.
	 */
	struct AllocationSpace
	{
		
	} __attribute__((packed));
}}}
