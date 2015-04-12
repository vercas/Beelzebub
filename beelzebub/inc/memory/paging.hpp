#pragma once

#include <handles.h>

namespace Beelzebub { namespace Memory { namespace Paging {
	const size_t PageSize = 4 * 1024;

	void InitializePageAllocator(uintptr_t const start, uintptr_t const end) blandfunc;

	Result ReservePageRange(size_t const start, size_t const count) blandfunc;
	Result ReservePhysicalRegion(size_t const start, size_t const length) blandfunc;
	
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
		PageDescriptor() blandfunc;
		PageDescriptor(const PageStatus st) blandfunc;

		/*	Status	*/
		PageStatus GetStatus() const blandfunc;
		PageStatus SetStatus(PageStatus const st) blandfunc;

		/*	Accesses	*/
		u8 GetAccesses() const blandfunc;
		void ResetAccesses() blandfunc;
		u8 IncrementAccesses() blandfunc;

		/*	Overalls	*/
		void Free() blandfunc;
		void Use() blandfunc;
		void Reserve() blandfunc;
	} __attribute__((packed));
}}}
