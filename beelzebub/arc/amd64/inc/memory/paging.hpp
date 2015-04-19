#pragma once

#include <handles.h>
#include <metaprogramming.h>

//	Creates a getter and setter for bit-based properties.
#define BITPROP(name)                                        \
__bland __forceinline bool MCATS2(Get, name)() const         \
{                                                            \
	return 0 != (this->Value & MCATS2(name, Bit));           \
}                                                            \
__bland __forceinline void MCATS2(Set, name)(const bool val) \
{                                                            \
	if (val)                                                 \
		this->Value |=  MCATS2(name, Bit);                   \
	else                                                     \
		this->Value &= ~MCATS2(name, Bit);                   \
}

namespace Beelzebub { namespace Memory { namespace Paging
{
/**
	 *	Page Map Level 1 Entry
	 */
	struct Pml1Entry
	{
		/*	Bit structure for 4-KiB page:
		 *		 0       : Present (if 1)
		 *		 1       : R/W (1 means writes allowed)
		 *		 2       : U/S (1 if usermode access allowed)
		 *		 3       : PWT (page-level write-through)
		 *		 4       : PCD (page-level cache disable)
		 *		 5       : Accessed
		 *		 6       : Dirty
		 *		 7       : PAT
		 *		 8       : Global
		 *		 9 -  11 : Ignored
		 *		12 - M-1 : Physical address of aligned 4-KiB page;
		 *		 M -  51 : Reserved (must be 0)
		 *		52 -  62 : Ignored
		 *		63       : XD (eXecute Disable, if 1)
		 */

		static const uint64_t PresentBit    = 1ULL <<  0;
		static const uint64_t WritableBit   = 1ULL <<  1;
		static const uint64_t UsermodeBit   = 1ULL <<  2;
		static const uint64_t PwtBit        = 1ULL <<  3;
		static const uint64_t PcdBit        = 1ULL <<  4;
		static const uint64_t AccessedBit   = 1ULL <<  5;
		static const uint64_t DirtyBit      = 1ULL <<  6;
		static const uint64_t PatBit        = 1ULL <<  7;
		static const uint64_t GlobalBit     = 1ULL <<  8;
		static const uint64_t XdBit         = 1ULL << 63;

		static const uint64_t AddressBits   = 0x000FFFFFFFFFF000ULL;

		/*  Constructors  */

		/**
		 *	Creates a new PML1 (PT) entry structure that maps a 4-KiB page.
		 */
		__bland __forceinline Pml1Entry(const void * const paddr
		                              , const bool         present
		                              , const bool         writable
		                              , const bool         userAccessible
		                              , const bool         PWT
		                              , const bool         PCD
		                              , const bool         accessed
		                              , const bool         dirty
		                              , const bool         global
		                              , const bool         PAT
		                              , const bool         XD)
		{
			this->Value = ((uint64_t)paddr & AddressBits)
			            | (present        ? PresentBit  : 0ULL)
			            | (writable       ? WritableBit : 0ULL)
			            | (userAccessible ? UsermodeBit : 0ULL)
			            | (PWT            ? PwtBit      : 0ULL)
			            | (PCD            ? PcdBit      : 0ULL)
			            | (accessed       ? AccessedBit : 0ULL)
			            | (dirty          ? DirtyBit    : 0ULL)
			            | (PAT            ? PatBit      : 0ULL)
			            | (global         ? GlobalBit   : 0ULL)
			            | (XD             ? XdBit       : 0ULL);
		}

		/*  Properties  */

		BITPROP(Present)
		BITPROP(Writable)
		BITPROP(Usermode)
		BITPROP(Pwt)
		BITPROP(Pcd)
		BITPROP(Accessed)
		BITPROP(Dirty)
		BITPROP(Pat)
		BITPROP(Global)
		BITPROP(Xd)

		/**
		 *	Gets the physical address of the 2-MiB page.
		 */
		__bland __forceinline void * GetAddress() const
		{
			return (void *)(this->Value & AddressBits);
		}
		/**
		 *	Sets the physical address of the 2-MiB page.
		 */
		__bland __forceinline void SetAddress(const void * const paddr)
		{
			this->Value = ((uint64_t)paddr       &  AddressBits)
			            | (          this->Value & ~AddressBits);
		}

		/*  Operators  */

		/**
		 *	Gets the value of a bit.
		 */
		__bland __forceinline bool operator[](const uint8_t bit)
		{
			return 0 != (this->Value & (1 << bit));
		}

	private:

		uint64_t Value;
	};
	typedef Pml1Entry PtEntry;

	/**
	 *	Page Map Level 1: Page Table.
	 */
	struct Pml1
	{
		/*
		 *	This table is indexed by bits 12-20 from the linear address.
		 *	Naturally, bits 0-2 of the index are 0. (8-byte entries)
		 */

		static const uint64_t AddressBits   = 0x00000000001FF000ULL;
		static const uint64_t IndexOffset   = 12;

		/*  Constructors  */

		//	I don't think I need constructors 'ere.

		/*  Operators  */

		/**
		 *	Gets the entrry at a given index.
		 */
		__bland __forceinline Pml1Entry& operator[](const uint16_t ind)
		{
			return this->Entries[ind];
		}

		/**
		 *	Gets the entry corresponding to the given linear address.
		 */
		__bland __forceinline Pml1Entry& operator[](const void * const laddr)
		{
			//	Take the interesting bits from the linear address...
			//	Shift it by the required amount of bits...
			//	And use that as an index! :D
			return this->Entries[((uint64_t)laddr & AddressBits) >> IndexOffset];
		}

		/*	Field(s)  */

	private:

		Pml1Entry Entries[512];	//	Yeah...
	};
	typedef Pml1 Pt;


	/**
	 *	Page Map Level 2 Entry
	 */
	struct Pml2Entry
	{
		/*	Bit structure for 2-MiB page:
		 *		 0       : Present (if 1)
		 *		 1       : R/W (1 means writes allowed)
		 *		 2       : U/S (1 if usermode access allowed)
		 *		 3       : PWT (page-level write-through)
		 *		 4       : PCD (page-level cache disable)
		 *		 5       : Accessed
		 *		 6       : Dirty
		 *		 7       : PageSize (must be 1)
		 *		 8       : Global
		 *		 9 -  11 : Ignored
		 *		12       : PAT
		 *		13 -  20 : Reserved (must be 0)
		 *		21 - M-1 : Physical address of aligned 2-MiB page;
		 *		 M -  51 : Reserved (must be 0)
		 *		52 -  62 : Ignored
		 *		63       : XD (eXecute Disable, if 1)
		 *
		 *	Bit structure for PML1 (PT) reference:
		 *		 0       : Present (if 1)
		 *		 1       : R/W (1 means writes allowed)
		 *		 2       : U/S (1 if usermode access allowed)
		 *		 3       : PWT (page-level write-through)
		 *		 4       : PCD (page-level cache disable)
		 *		 5       : Accessed
		 *		 6       : Ignored
		 *		 7       : PageSize (must be 0)
		 *		 8 -  11 : Ignored
		 *		12 - M-1 : Physical address of PML1 (PT) table; 4-KiB aligned.
		 *		 M -  51 : Reserved (must be 0)
		 *		52 -  62 : Ignored
		 *		63       : XD (eXecute Disable, if 1)
		 */

		static const uint64_t PresentBit    = 1ULL <<  0;
		static const uint64_t WritableBit   = 1ULL <<  1;
		static const uint64_t UsermodeBit   = 1ULL <<  2;
		static const uint64_t PwtBit        = 1ULL <<  3;
		static const uint64_t PcdBit        = 1ULL <<  4;
		static const uint64_t AccessedBit   = 1ULL <<  5;
		static const uint64_t DirtyBit      = 1ULL <<  6;
		static const uint64_t PageSizeBit   = 1ULL <<  7;
		static const uint64_t GlobalBit     = 1ULL <<  8;
		static const uint64_t PatBit        = 1ULL << 12;
		static const uint64_t XdBit         = 1ULL << 63;

		static const uint64_t AddressBits   = 0x000FFFFFFFE00000ULL;
		static const uint64_t ReferenceBits = 0x000FFFFFFFFFF000ULL;

		/*  Constructors  */

		/**
		 *	Creates a new PML2 (PD) entry structure that points to a PML1 (PT) table.
		 */
		__bland __forceinline Pml2Entry(const Pml1 * const pml1_paddr
		                              , const bool         present
		                              , const bool         writable
		                              , const bool         userAccessible
		                              , const bool         PWT
		                              , const bool         PCD
		                              , const bool         accessed
		                              , const bool         XD)
		{
			this->Value = ((uint64_t)pml1_paddr & ReferenceBits)
			            | (present        ? PresentBit  : 0ULL)
			            | (writable       ? WritableBit : 0ULL)
			            | (userAccessible ? UsermodeBit : 0ULL)
			            | (PWT            ? PwtBit      : 0ULL)
			            | (PCD            ? PcdBit      : 0ULL)
			            | (accessed       ? AccessedBit : 0ULL)
			            | (XD             ? XdBit       : 0ULL);
		}

		/**
		 *	Creates a new PML2 (PD) entry structure that maps a 2-MiB page.
		 */
		__bland __forceinline Pml2Entry(const void * const paddr
		                              , const bool         present
		                              , const bool         writable
		                              , const bool         userAccessible
		                              , const bool         PWT
		                              , const bool         PCD
		                              , const bool         accessed
		                              , const bool         dirty
		                              , const bool         global
		                              , const bool         PAT
		                              , const bool         XD)
		{
			this->Value = ((uint64_t)paddr & AddressBits)
			            | (present        ? PresentBit  : 0ULL)
			            | (writable       ? WritableBit : 0ULL)
			            | (userAccessible ? UsermodeBit : 0ULL)
			            | (PWT            ? PwtBit      : 0ULL)
			            | (PCD            ? PcdBit      : 0ULL)
			            | (accessed       ? AccessedBit : 0ULL)
			            | (dirty          ? DirtyBit    : 0ULL)
			            | (global         ? GlobalBit   : 0ULL)
			            | (PAT            ? PatBit      : 0ULL)
			            | (XD             ? XdBit       : 0ULL)
			            |                   PageSizeBit        ;
		}

		/*  Properties  */

		BITPROP(Present)
		BITPROP(Writable)
		BITPROP(Usermode)
		BITPROP(Pwt)
		BITPROP(Pcd)
		BITPROP(Accessed)
		BITPROP(Dirty)
		BITPROP(PageSize)
		BITPROP(Global)
		BITPROP(Pat)
		BITPROP(Xd)

		/**
		 *	Gets the physical address of the PML1 (PT) table.
		 */
		__bland __forceinline Pml1 * GetPml1Ptr() const
		{
			return (Pml1 *)(this->Value & ReferenceBits);
		}
		/**
		 *	Sets the physical address of the PML1 (PT) table.
		 */
		__bland __forceinline void SetPml1Ptr(const Pml1 * const paddr)
		{
			this->Value = ((uint64_t)paddr       &  ReferenceBits)
			            | (          this->Value & ~ReferenceBits);
		}

		/**
		 *	Gets the physical address of the 2-MiB page.
		 */
		__bland __forceinline void * GetAddress() const
		{
			return (void *)(this->Value & AddressBits);
		}
		/**
		 *	Sets the physical address of the 2-MiB page.
		 */
		__bland __forceinline void SetAddress(const void * const paddr)
		{
			this->Value = ((uint64_t)paddr       &  AddressBits)
			            | (          this->Value & ~AddressBits);
		}

		/*  Operators  */

		/**
		 *	Gets the value of a bit.
		 */
		__bland __forceinline bool operator[](const uint8_t bit)
		{
			return 0 != (this->Value & (1 << bit));
		}

	private:

		uint64_t Value;
	};
	typedef Pml2Entry PdEntry;

	/**
	 *	Page Map Level 2: Page Directory.
	 */
	struct Pml2
	{
		/*
		 *	This table is indexed by bits 21-29 from the linear address.
		 *	Naturally, bits 0-2 of the index are 0. (8-byte entries)
		 */

		static const uint64_t AddressBits   = 0x000000003FE00000ULL;
		static const uint64_t IndexOffset   = 21;

		/*  Constructors  */

		//	I don't think I need constructors 'ere.

		/*  Operators  */

		/**
		 *	Gets the entrry at a given index.
		 */
		__bland __forceinline Pml2Entry& operator[](const uint16_t ind)
		{
			return this->Entries[ind];
		}

		/**
		 *	Gets the entry corresponding to the given linear address.
		 */
		__bland __forceinline Pml2Entry& operator[](const void * const laddr)
		{
			//	Take the interesting bits from the linear address...
			//	Shift it by the required amount of bits...
			//	And use that as an index! :D
			return this->Entries[((uint64_t)laddr & AddressBits) >> IndexOffset];
		}

		/*	Field(s)  */

	private:

		Pml2Entry Entries[512];	//	Yeah...
	};
	typedef Pml2 Pd;


	/**
	 *	Page Map Level 3 Entry
	 */
	struct Pml3Entry
	{
		/*	Bit structure for 1-GiB page:
		 *		 0       : Present (if 1)
		 *		 1       : R/W (1 means writes allowed)
		 *		 2       : U/S (1 if usermode access allowed)
		 *		 3       : PWT (page-level write-through)
		 *		 4       : PCD (page-level cache disable)
		 *		 5       : Accessed
		 *		 6       : Dirty
		 *		 7       : PageSize (must be 1)
		 *		 8       : Global
		 *		 9 -  11 : Ignored
		 *		12       : PAT
		 *		13 -  29 : Reserved (must be 0)
		 *		30 - M-1 : Physical address of aligned 1-GiB page.
		 *		 M -  51 : Reserved (must be 0)
		 *		52 -  62 : Ignored
		 *		63       : XD (eXecute Disable, if 1)
		 *
		 *	Bit structure for PML2 reference:
		 *		 0       : Present (if 1)
		 *		 1       : R/W (1 means writes allowed)
		 *		 2       : U/S (1 if usermode access allowed)
		 *		 3       : PWT (page-level write-through)
		 *		 4       : PCD (page-level cache disable)
		 *		 5       : Accessed
		 *		 6       : Ignored
		 *		 7       : PageSize (must be 0)
		 *		 8 -  11 : Ignored
		 *		12 - M-1 : Physical address of PML2 (PD) table; 4-KiB aligned.
		 *		 M -  51 : Reserved (must be 0)
		 *		52 -  62 : Ignored
		 *		63       : XD (eXecute Disable, if 1)
		 */

		static const uint64_t PresentBit    = 1ULL <<  0;
		static const uint64_t WritableBit   = 1ULL <<  1;
		static const uint64_t UsermodeBit   = 1ULL <<  2;
		static const uint64_t PwtBit        = 1ULL <<  3;
		static const uint64_t PcdBit        = 1ULL <<  4;
		static const uint64_t AccessedBit   = 1ULL <<  5;
		static const uint64_t DirtyBit      = 1ULL <<  6;
		static const uint64_t PageSizeBit   = 1ULL <<  7;
		static const uint64_t GlobalBit     = 1ULL <<  8;
		static const uint64_t PatBit        = 1ULL << 12;
		static const uint64_t XdBit         = 1ULL << 63;

		static const uint64_t AddressBits   = 0x000FFFFFC0000000ULL;
		static const uint64_t ReferenceBits = 0x000FFFFFFFFFF000ULL;

		/*  Constructors  */

		/**
		 *	Creates a new PML3 entry structure that points to a PML2 table.
		 */
		__bland __forceinline Pml3Entry(const Pml2 * const pml2_paddr
		                              , const bool         present
		                              , const bool         writable
		                              , const bool         userAccessible
		                              , const bool         PWT
		                              , const bool         PCD
		                              , const bool         accessed
		                              , const bool         XD)
		{
			this->Value = ((uint64_t)pml2_paddr & ReferenceBits)
			            | (present        ? PresentBit  : 0ULL)
			            | (writable       ? WritableBit : 0ULL)
			            | (userAccessible ? UsermodeBit : 0ULL)
			            | (PWT            ? PwtBit      : 0ULL)
			            | (PCD            ? PcdBit      : 0ULL)
			            | (accessed       ? AccessedBit : 0ULL)
			            | (XD             ? XdBit       : 0ULL);
		}

		/**
		 *	Creates a new PML3 entry structure that points to a 1-GiB page.
		 */
		__bland __forceinline Pml3Entry(const void * const paddr
		                              , const bool         present
		                              , const bool         writable
		                              , const bool         userAccessible
		                              , const bool         PWT
		                              , const bool         PCD
		                              , const bool         accessed
		                              , const bool         dirty
		                              , const bool         global
		                              , const bool         PAT
		                              , const bool         XD)
		{
			this->Value = ((uint64_t)paddr & AddressBits)
			            | (present        ? PresentBit  : 0ULL)
			            | (writable       ? WritableBit : 0ULL)
			            | (userAccessible ? UsermodeBit : 0ULL)
			            | (PWT            ? PwtBit      : 0ULL)
			            | (PCD            ? PcdBit      : 0ULL)
			            | (accessed       ? AccessedBit : 0ULL)
			            | (dirty          ? DirtyBit    : 0ULL)
			            | (global         ? GlobalBit   : 0ULL)
			            | (PAT            ? PatBit      : 0ULL)
			            | (XD             ? XdBit       : 0ULL)
			            |                   PageSizeBit        ;
		}

		/*  Properties  */

		BITPROP(Present)
		BITPROP(Writable)
		BITPROP(Usermode)
		BITPROP(Pwt)
		BITPROP(Pcd)
		BITPROP(Accessed)
		BITPROP(Dirty)
		BITPROP(PageSize)
		BITPROP(Global)
		BITPROP(Pat)
		BITPROP(Xd)

		/**
		 *	Gets the physical address of the PML2 (PD) table.
		 */
		__bland __forceinline Pml2 * GetPml2Ptr() const
		{
			return (Pml2 *)(this->Value & ReferenceBits);
		}
		/**
		 *	Sets the physical address of the PML2 (PD) table.
		 */
		__bland __forceinline void SetPml2Ptr(const Pml2 * const paddr)
		{
			this->Value = ((uint64_t)paddr       &  ReferenceBits)
			            | (          this->Value & ~ReferenceBits);
		}

		/**
		 *	Gets the physical address of the 1-GiB page.
		 */
		__bland __forceinline void * GetAddress() const
		{
			return (void *)(this->Value & AddressBits);
		}
		/**
		 *	Sets the physical address of the 1-GiB page.
		 */
		__bland __forceinline void SetAddress(const void * const paddr)
		{
			this->Value = ((uint64_t)paddr       &  AddressBits)
			            | (          this->Value & ~AddressBits);
		}

		/*  Operators  */

		/**
		 *	Gets the value of a bit.
		 */
		__bland __forceinline bool operator[](const uint8_t bit)
		{
			return 0 != (this->Value & (1 << bit));
		}

	private:

		uint64_t Value;
	};
	typedef Pml3Entry PdptEntry;

	/**
	 *	Page Map Level 3: Page Directory Pointer Table.
	 */
	struct Pml3
	{
		/*
		 *	This table is indexed by bits 30-38 from the linear address.
		 *	Naturally, bits 0-2 of the index are 0. (8-byte entries)
		 */

		static const uint64_t AddressBits   = 0x0000007FC0000000ULL;
		static const uint64_t IndexOffset   = 30;

		/*  Constructors  */

		//	I don't think I need constructors 'ere.

		/*  Operators  */

		/**
		 *	Gets the entrry at a given index.
		 */
		__bland __forceinline Pml3Entry& operator[](const uint16_t ind)
		{
			return this->Entries[ind];
		}

		/**
		 *	Gets the entry corresponding to the given linear address.
		 */
		__bland __forceinline Pml3Entry& operator[](const void * const laddr)
		{
			//	Take the interesting bits from the linear address...
			//	Shift it by the required amount of bits...
			//	And use that as an index! :D
			return this->Entries[((uint64_t)laddr & AddressBits) >> IndexOffset];
		}

		/*	Field(s)  */

	private:

		Pml3Entry Entries[512];	//	Yeah...
	};
	typedef Pml3 Pdpt;


	/**
	 *	Page Map Level 4 Entry
	 */
	struct Pml4Entry
	{
		/*	Bit structure:
		 *		 0       : Present (if 1)
		 *		 1       : R/W (1 means writes allowed)
		 *		 2       : U/S (1 if usermode access allowed)
		 *		 3       : PWT (page-level write-through)
		 *		 4       : PCD (page-level cache disable)
		 *		 5       : Accessed
		 *		 6       : Ignored
		 *		 7       : Reserved (must be 0)
		 *		 8 -  11 : Ignored
		 *		12 - M-1 : Physical address of PML3(PDPT) table; 4-KiB aligned.
		 *		 M -  51 : Reserved (must be 0)
		 *		52 -  62 : Ignored
		 *		63       : XD (eXecute Disable, if 1)
		 */

		static const uint64_t PresentBit    = 1ULL <<  0;
		static const uint64_t WritableBit   = 1ULL <<  1;
		static const uint64_t UsermodeBit   = 1ULL <<  2;
		static const uint64_t PwtBit        = 1ULL <<  3;
		static const uint64_t PcdBit        = 1ULL <<  4;
		static const uint64_t AccessedBit   = 1ULL <<  5;
		static const uint64_t XdBit         = 1ULL << 63;

		static const uint64_t ReferenceBits = 0x000FFFFFFFFFF000ULL;

		/*  Constructors  */

		/**
		 *	Creates a new empty PML4 entry structure.
		 */
		__bland __forceinline Pml4Entry()
		{
			this->Value = 0ULL;
		}

		/**
		 *	Creates a new PML4 entry structure that points to a PML3 table.
		 */
		__bland __forceinline Pml4Entry(const Pml3 * const pml3_paddr
		                              , const bool         present
		                              , const bool         writable
		                              , const bool         userAccessible
		                              , const bool         PWT
		                              , const bool         PCD
		                              , const bool         accessed
		                              , const bool         XD)
		{
			this->Value = ((uint64_t)pml3_paddr & ReferenceBits)
			            | (present        ? PresentBit  : 0ULL)
			            | (writable       ? WritableBit : 0ULL)
			            | (userAccessible ? UsermodeBit : 0ULL)
			            | (PWT            ? PwtBit      : 0ULL)
			            | (PCD            ? PcdBit      : 0ULL)
			            | (accessed       ? AccessedBit : 0ULL)
			            | (XD             ? XdBit       : 0ULL);
		}

		/*  Properties  */

		BITPROP(Present)
		BITPROP(Writable)
		BITPROP(Usermode)
		BITPROP(Pwt)
		BITPROP(Pcd)
		BITPROP(Accessed)
		BITPROP(Xd)

		/**
		 *	Gets the physical address of the PML3 (PDPT) table.
		 */
		__bland __forceinline Pml3 * GetPml3Ptr() const
		{
			return (Pml3 *)(this->Value & ReferenceBits);
		}
		/**
		 *	Sets the physical address of the PML3 (PDPT) table.
		 */
		__bland __forceinline void SetPml3Ptr(const Pml3 * const paddr)
		{
			this->Value = ((uint64_t)paddr       &  ReferenceBits)
			            | (          this->Value & ~ReferenceBits);
		}

		/*  Operators  */

		/**
		 *	Gets the value of a bit.
		 */
		__bland __forceinline bool operator[](const uint8_t bit)
		{
			return 0 != (this->Value & (1 << bit));
		}

	private:

		uint64_t Value;
	};

	/**
	 *	Page Map Level 4
	 */
	struct Pml4
	{
		/*
		 *	This table is indexed by bits 39-47 from the linear address.
		 *	Naturally, bits 0-2 of the index are 0. (8-byte entries)
		 */

		static const uint64_t AddressBits   = 0x0000FF8000000000ULL;
		static const uint64_t IndexOffset   = 39;

		/*  Constructors  */

		// constructor goes here lel

		/*  Operators  */

		/**
		 *	Gets the entrry at a given index.
		 */
		__bland __forceinline Pml4Entry& operator[](const uint16_t ind)
		{
			return this->Entries[ind];
		}

		/**
		 *	Gets the entry corresponding to the given linear address.
		 */
		__bland __forceinline Pml4Entry& operator[](const void * const laddr)
		{
			//	Take the interesting bits from the linear address...
			//	Shift it by the required amount of bits...
			//	And use that as an index! :D
			return this->Entries[((uint64_t)laddr & AddressBits) >> IndexOffset];
		}

		/*	Field(s)  */

	private:

		Pml4Entry Entries[512];	//	Yeah...
	};


	/**
	 * Represents the contents of the CR3 register.
	 */
	struct Cr3
	{
		/*
		 *	Bit structure with PCID disabled:
		 *		 0 -   2 : Ignored
		 *		 3       : PWT (page-level write-through)
		 *		 4       : PCD (page-level cache disable)
		 *		 5 -  11 : Ignored
		 *		12 - M-1 : Physical address of PML4 table; 4-KiB aligned.
		 *		 M -  63 : Reserved (must be 0)
		 *
		 *	Bit structure with PCID enabled:
		 *		 0 -  11 : PCID
		 *		12 - M-1 : Physical address of PML4 table; 4-KiB aligned.
		 *		 M -  63 : Reserved (must be 0)
		 */

		static const uint64_t PwtBit        = 1ULL <<  3;
		static const uint64_t PcdBit        = 1ULL <<  4;

		static const uint64_t ReferenceBits = 0x000FFFFFFFFFF000ULL;
		static const uint64_t PcidBits      = 0x0000000000000FFFULL;

		/*  Constructors  */

		/**
		 *	Creates a new CR3 structure from the given raw value.
		 */
		__bland __forceinline Cr3(const size_t val)
		{
			this->Value = val;
		}

		/**
		 *	Creates a new CR3 structure for use with PCID disabled.
		 */
		__bland __forceinline Cr3(const Pml4 * const paddr, const bool PWT, const bool PCD)
		{
			this->Value = ((uint64_t)paddr & ReferenceBits)
			            | (PWT ? PwtBit : 0)
			            | (PCD ? PcdBit : 0);
		}

		/**
		 *	Creates a new CR3 structure for use with PCID enabled.
		 */
		__bland __forceinline Cr3(const Pml4 * const paddr, const Handle process)
		{
			//	TODO: Check whether the handle is correct or not.

			this->Value = ((uint64_t)paddr         & ReferenceBits)
			            | ((uint64_t)process.Value & PcidBits     );
		}

		/*  Properties  */

		BITPROP(Pwt)
		BITPROP(Pcd)

		/**
		 *	Gets the physical address of the PML4 table.
		 */
		__bland __forceinline Pml4 * GetPml4Ptr() const
		{
			return (Pml4 *)(this->Value & ReferenceBits);
		}
		/**
		 *	Sets the physical address of the PML4 table.
		 */
		__bland __forceinline void SetPml4Ptr(const Pml4 * const paddr)
		{
			this->Value = ((uint64_t)paddr       &  ReferenceBits)
			            | (          this->Value & ~ReferenceBits);
		}

		/**
		 *	Gets the PCID value.
		 */
		__bland __forceinline Handle GetPcid() const
		{
			//	TODO: Construct proper handles here!

			return { this->Value & PcidBits };
		}
		/**
		 *	Sets the PCID value from the given process.
		 */
		__bland __forceinline void SetPcid(const Handle process)
		{
			//	TODO: Check whether the handle is correct or not.

			this->Value = (process.Value &  PcidBits)
			            | (  this->Value & ~PcidBits);
		}

		/*  Operators  */

		/**
		 *	Gets the value of a bit.
		 */
		__bland __forceinline bool operator[](const uint8_t bit)
		{
			return 0 != (this->Value & (1 << bit));
		}

		/*	Field(s)  */

	private:

		size_t Value;
	};
}}}