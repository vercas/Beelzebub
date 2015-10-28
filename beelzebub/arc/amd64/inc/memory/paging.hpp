/**
 *  Bit 11 of all entries is used as a spinlock!
 */

#pragma once

#include <utils/bitfields.hpp>

//  Creates the functions necessary for operating a spinlock over an entry.
#define SPINLOCK(name, bit)                                  \
__bland __forceinline bool MCATS2(TryAcquire, name)()        \
{                                                            \
    bool res = 0;                                            \
                                                             \
    asm volatile ("lock btsq $" #bit ", %[val] \n\t"         \
                  "setnc %[res] \n\t"                        \
                 : [res] "=r" (res)                          \
                 : [val] "m" (this->Value)                   \
                 : "cc");                                    \
                                                             \
    return res;                                              \
}                                                            \
__bland __forceinline void MCATS2(Spin, name)()              \
{                                                            \
    do                                                       \
    {                                                        \
        asm volatile ("pause");                              \
    } while (!this->MCATS2(Check, name)());                  \
}                                                            \
__bland __forceinline void MCATS2(Await, name)()             \
{                                                            \
    while (!this->MCATS2(Check, name)())                     \
    {                                                        \
        asm volatile ("pause");                              \
    }                                                        \
}                                                            \
__bland __forceinline void MCATS2(Acquire, name)()           \
{                                                            \
    while (!this->MCATS2(TryAcquire, name)())                \
        this->MCATS2(Spin, name)();                          \
}                                                            \
__bland __forceinline void MCATS2(Release, name)()           \
{                                                            \
    asm volatile ("lock btrq $" #bit ", %[val] \n\t"         \
                 :                                           \
                 : [val] "m" (this->Value)                   \
                 : "cc");                                    \
}                                                            \
__bland __forceinline bool MCATS2(Check, name)()             \
{                                                            \
    return 0 == (this->Value & (1 << bit));                  \
    /*  A simple AND operation works very well here.  */     \
}

namespace Beelzebub { namespace Memory
{
    /**
     *  Page Map Level 1 Entry
     */
    struct Pml1Entry
    {
        /*  Bit structure for 4-KiB page:
         *       0       : Present (if 1)
         *       1       : R/W (1 means writes allowed)
         *       2       : U/S (1 if usermode access allowed)
         *       3       : PWT (page-level write-through)
         *       4       : PCD (page-level cache disable)
         *       5       : Accessed
         *       6       : Dirty
         *       7       : PAT
         *       8       : Global
         *       9 -  11 : Ignored
         *      12 - M-1 : Physical address of aligned 4-KiB page;
         *       M -  51 : Reserved (must be 0)
         *      52 -  62 : Ignored
         *      63       : XD (eXecute Disable, if 1)
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1W( 0, Present )
        BITFIELD_DEFAULT_1W( 1, Writable)
        BITFIELD_DEFAULT_1W( 2, Userland)
        BITFIELD_DEFAULT_1W( 3, Pwt     )
        BITFIELD_DEFAULT_1W( 4, Pcd     )
        BITFIELD_DEFAULT_1W( 5, Accessed)
        BITFIELD_DEFAULT_1W( 6, Dirty   )
        BITFIELD_DEFAULT_1W( 7, Pat     )
        BITFIELD_DEFAULT_1W( 8, Global  )
        BITFIELD_DEFAULT_1W(63, Xd      )
        BITFIELD_DEFAULT_2W(12, 40, paddr_t, Address)
        
        /*  Constructors  */

        /**
         *  Creates an empty PML1 (PT) entry structure.
         */
        __bland inline Pml1Entry() : Value( 0ULL ) { }

        /**
         *  Creates a new PML1 (PT) entry structure that maps a 4-KiB page.
         */
        __bland inline Pml1Entry(const paddr_t paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    global
                               , const bool    XD)
            : Value((paddr & AddressBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (global         ? GlobalBit   : 0ULL)
                  | (XD             ? XdBit       : 0ULL))
        {

        }

        /**
         *  Creates a new PML1 (PT) entry structure that maps a 4-KiB page.
         */
        __bland inline Pml1Entry(const paddr_t paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    PWT
                               , const bool    PCD
                               , const bool    accessed
                               , const bool    dirty
                               , const bool    global
                               , const bool    PAT
                               , const bool    XD)
            : Value((paddr & AddressBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (PWT            ? PwtBit      : 0ULL)
                  | (PCD            ? PcdBit      : 0ULL)
                  | (accessed       ? AccessedBit : 0ULL)
                  | (dirty          ? DirtyBit    : 0ULL)
                  | (PAT            ? PatBit      : 0ULL)
                  | (global         ? GlobalBit   : 0ULL)
                  | (XD             ? XdBit       : 0ULL))
        {

        }

        /*  Properties  */

        __bland inline bool IsNull()
        {
            return (this->GetAddress() == nullpaddr) && !this->GetPresent();
        }

        /*  Synchronization  */

        SPINLOCK(ContentLock, 10);
        SPINLOCK(PropertiesLock, 11);

    //private:

        uint64_t Value;
    };
    typedef Pml1Entry PtEntry;

    /**
     *  Page Map Level 1: Page Table.
     */
    struct Pml1
    {
        /*
         *  This table is indexed by bits 12-20 from the linear address.
         *  Naturally, bits 0-2 of the index are 0. (8-byte entries)
         */

        static const uint64_t AddressBits   = 0x00000000001FF000ULL;
        static const uint64_t IndexOffset   = 12;

        /*  Constructors  */

        Pml1() = default;
        Pml1(Pml1 const &) = default;

        /*  Operators  */

        /**
         *  Gets the entrry at a given index.
         */
        __bland inline Pml1Entry & operator [](uint16_t const ind)
        {
            return this->Entries[ind];
        }

        /**
         *  Gets the entry corresponding to the given linear address.
         */
        __bland inline Pml1Entry & operator [](vaddrptr_t const vaddr)
        {
            //  Take the interesting bits from the linear address...
            //  Shift it by the required amount of bits...
            //  And use that as an index! :D
            return this->Entries[(vaddr.val & AddressBits) >> IndexOffset];
        }

        /*  Field(s)  */

    //private:

        Pml1Entry Entries[512]; //  Yeah...
    };
    typedef Pml1 Pt;


    /**
     *  Page Map Level 2 Entry
     */
    struct Pml2Entry
    {
        /*  Bit structure for 2-MiB page:
         *       0       : Present (if 1)
         *       1       : R/W (1 means writes allowed)
         *       2       : U/S (1 if usermode access allowed)
         *       3       : PWT (page-level write-through)
         *       4       : PCD (page-level cache disable)
         *       5       : Accessed
         *       6       : Dirty
         *       7       : PageSize (must be 1)
         *       8       : Global
         *       9 -  11 : Ignored
         *      12       : PAT
         *      13 -  20 : Reserved (must be 0)
         *      21 - M-1 : Physical address of aligned 2-MiB page;
         *       M -  51 : Reserved (must be 0)
         *      52 -  62 : Ignored
         *      63       : XD (eXecute Disable, if 1)
         *
         *  Bit structure for PML1 (PT) reference:
         *       0       : Present (if 1)
         *       1       : R/W (1 means writes allowed)
         *       2       : U/S (1 if usermode access allowed)
         *       3       : PWT (page-level write-through)
         *       4       : PCD (page-level cache disable)
         *       5       : Accessed
         *       6       : Ignored
         *       7       : PageSize (must be 0)
         *       8 -  11 : Ignored
         *      12 - M-1 : Physical address of PML1 (PT) table; 4-KiB aligned.
         *       M -  51 : Reserved (must be 0)
         *      52 -  62 : Ignored
         *      63       : XD (eXecute Disable, if 1)
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1W( 0, Present )
        BITFIELD_DEFAULT_1W( 1, Writable)
        BITFIELD_DEFAULT_1W( 2, Userland)
        BITFIELD_DEFAULT_1W( 3, Pwt     )
        BITFIELD_DEFAULT_1W( 4, Pcd     )
        BITFIELD_DEFAULT_1W( 5, Accessed)
        BITFIELD_DEFAULT_1W( 6, Dirty   )
        BITFIELD_DEFAULT_1W( 7, PageSize)
        BITFIELD_DEFAULT_1W( 8, Global  )
        BITFIELD_DEFAULT_1W(12, Pat     )
        BITFIELD_DEFAULT_1W(63, Xd      )
        BITFIELD_DEFAULT_2W(12, 40, Pml1 * , Pml1Ptr)
        BITFIELD_DEFAULT_2W(12, 40, paddr_t, Pml1Address)
        BITFIELD_DEFAULT_2W(21, 31, paddr_t, Address)

        /*  Constructors  */

        /**
         *  Creates an empty PML2 (PD) entry structure.
         */
        __bland inline Pml2Entry() : Value( 0ULL ) { }

        /**
         *  Creates a new PML2 (PD) entry structure that points to a PML1 (PT) table.
         */
        __bland inline Pml2Entry(const paddr_t pml1_paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    XD)
            : Value((pml1_paddr & Pml1PtrBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (XD             ? XdBit       : 0ULL))
        {

        }

        /**
         *  Creates a new PML2 (PD) entry structure that points to a PML1 (PT) table.
         */
        __bland inline Pml2Entry(const paddr_t pml1_paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    PWT
                               , const bool    PCD
                               , const bool    accessed
                               , const bool    XD)
            : Value((pml1_paddr & Pml1PtrBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (PWT            ? PwtBit      : 0ULL)
                  | (PCD            ? PcdBit      : 0ULL)
                  | (accessed       ? AccessedBit : 0ULL)
                  | (XD             ? XdBit       : 0ULL))
        {

        }

        /**
         *  Creates a new PML2 (PD) entry structure that maps a 2-MiB page.
         */
        __bland inline Pml2Entry(const paddr_t paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    global
                               , const bool    XD)
            : Value(((uint64_t)paddr & AddressBits)
                    | (present        ? PresentBit  : 0ULL)
                    | (writable       ? WritableBit : 0ULL)
                    | (userAccessible ? UserlandBit : 0ULL)
                    | (global         ? GlobalBit   : 0ULL)
                    | (XD             ? XdBit       : 0ULL)
                    |                   PageSizeBit        )
        {

        }

        /**
         *  Creates a new PML2 (PD) entry structure that maps a 2-MiB page.
         */
        __bland inline Pml2Entry(const paddr_t paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    PWT
                               , const bool    PCD
                               , const bool    accessed
                               , const bool    dirty
                               , const bool    global
                               , const bool    PAT
                               , const bool    XD)
            : Value(((uint64_t)paddr & AddressBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (PWT            ? PwtBit      : 0ULL)
                  | (PCD            ? PcdBit      : 0ULL)
                  | (accessed       ? AccessedBit : 0ULL)
                  | (dirty          ? DirtyBit    : 0ULL)
                  | (global         ? GlobalBit   : 0ULL)
                  | (PAT            ? PatBit      : 0ULL)
                  | (XD             ? XdBit       : 0ULL)
                  |                   PageSizeBit        )
        {

        }

        /*  Properties  */

        __bland inline bool IsNull()
        {
            return (this->GetPml1Address() == nullpaddr) && !this->GetPresent();
        }

        /*  Synchronization  */

        SPINLOCK(ContentLock, 10);
        SPINLOCK(PropertiesLock, 11);

    //private:

        uint64_t Value;
    };
    typedef Pml2Entry PdEntry;

    /**
     *  Page Map Level 2: Page Directory.
     */
    struct Pml2
    {
        /*
         *  This table is indexed by bits 21-29 from the linear address.
         *  Naturally, bits 0-2 of the index are 0. (8-byte entries)
         */

        static const uint64_t AddressBits   = 0x000000003FE00000ULL;
        static const uint64_t IndexOffset   = 21;

        /*  Constructors  */

        Pml2() = default;
        Pml2(Pml2 const &) = default;

        /*  Operators  */

        /**
         *  Gets the entrry at a given index.
         */
        __bland inline Pml2Entry & operator [](uint16_t const ind)
        {
            return this->Entries[ind];
        }

        /**
         *  Gets the entry corresponding to the given linear address.
         */
        __bland inline Pml2Entry & operator [](vaddrptr_t const vaddr)
        {
            //  Take the interesting bits from the linear address...
            //  Shift it by the required amount of bits...
            //  And use that as an index! :D
            return this->Entries[(vaddr.val & AddressBits) >> IndexOffset];
        }

        /*  Field(s)  */

    //private:

        Pml2Entry Entries[512]; //  Yeah...
    };
    typedef Pml2 Pd;


    /**
     *  Page Map Level 3 Entry
     */
    struct Pml3Entry
    {
        /*  Bit structure for 1-GiB page:
         *       0       : Present (if 1)
         *       1       : R/W (1 means writes allowed)
         *       2       : U/S (1 if usermode access allowed)
         *       3       : PWT (page-level write-through)
         *       4       : PCD (page-level cache disable)
         *       5       : Accessed
         *       6       : Dirty
         *       7       : PageSize (must be 1)
         *       8       : Global
         *       9 -  11 : Ignored
         *      12       : PAT
         *      13 -  29 : Reserved (must be 0)
         *      30 - M-1 : Physical address of aligned 1-GiB page.
         *       M -  51 : Reserved (must be 0)
         *      52 -  62 : Ignored
         *      63       : XD (eXecute Disable, if 1)
         *
         *  Bit structure for PML2 reference:
         *       0       : Present (if 1)
         *       1       : R/W (1 means writes allowed)
         *       2       : U/S (1 if usermode access allowed)
         *       3       : PWT (page-level write-through)
         *       4       : PCD (page-level cache disable)
         *       5       : Accessed
         *       6       : Ignored
         *       7       : PageSize (must be 0)
         *       8 -  11 : Ignored
         *      12 - M-1 : Physical address of PML2 (PD) table; 4-KiB aligned.
         *       M -  51 : Reserved (must be 0)
         *      52 -  62 : Ignored
         *      63       : XD (eXecute Disable, if 1)
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1W( 0, Present )
        BITFIELD_DEFAULT_1W( 1, Writable)
        BITFIELD_DEFAULT_1W( 2, Userland)
        BITFIELD_DEFAULT_1W( 3, Pwt     )
        BITFIELD_DEFAULT_1W( 4, Pcd     )
        BITFIELD_DEFAULT_1W( 5, Accessed)
        BITFIELD_DEFAULT_1W( 6, Dirty   )
        BITFIELD_DEFAULT_1W( 7, PageSize)
        BITFIELD_DEFAULT_1W( 8, Global  )
        BITFIELD_DEFAULT_1W(12, Pat     )
        BITFIELD_DEFAULT_1W(63, Xd      )
        BITFIELD_DEFAULT_2W(12, 40, Pml2 * , Pml2Ptr)
        BITFIELD_DEFAULT_2W(12, 40, paddr_t, Pml2Address)
        BITFIELD_DEFAULT_2W(30, 22, paddr_t, Address)

        /*  Constructors  */

        /**
         *  Creates an empty PML3 (PDPT) entry structure.
         */
        __bland inline Pml3Entry() : Value( 0ULL ) { }

        /**
         *  Creates a new PML3 entry structure that points to a PML2 table.
         */
        __bland inline Pml3Entry(const paddr_t pml2_paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    XD)
            : Value((pml2_paddr & Pml2PtrBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (XD             ? XdBit       : 0ULL))
        {

        }

        /**
         *  Creates a new PML3 entry structure that points to a PML2 table.
         */
        __bland inline Pml3Entry(const paddr_t pml2_paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    PWT
                               , const bool    PCD
                               , const bool    accessed
                               , const bool    XD)
            : Value((pml2_paddr & Pml2PtrBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (PWT            ? PwtBit      : 0ULL)
                  | (PCD            ? PcdBit      : 0ULL)
                  | (accessed       ? AccessedBit : 0ULL)
                  | (XD             ? XdBit       : 0ULL))
        {

        }

        /**
         *  Creates a new PML3 entry structure that points to a 1-GiB page.
         */
        __bland inline Pml3Entry(const paddr_t paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    global
                               , const bool    XD)
            : Value((paddr & AddressBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (global         ? GlobalBit   : 0ULL)
                  | (XD             ? XdBit       : 0ULL)
                  |                   PageSizeBit        )
        {

        }

        /**
         *  Creates a new PML3 entry structure that points to a 1-GiB page.
         */
        __bland inline Pml3Entry(const paddr_t paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    PWT
                               , const bool    PCD
                               , const bool    accessed
                               , const bool    dirty
                               , const bool    global
                               , const bool    PAT
                               , const bool    XD)
            : Value((paddr & AddressBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (PWT            ? PwtBit      : 0ULL)
                  | (PCD            ? PcdBit      : 0ULL)
                  | (accessed       ? AccessedBit : 0ULL)
                  | (dirty          ? DirtyBit    : 0ULL)
                  | (global         ? GlobalBit   : 0ULL)
                  | (PAT            ? PatBit      : 0ULL)
                  | (XD             ? XdBit       : 0ULL)
                  |                   PageSizeBit        )
        {

        }

        /*  Properties  */

        __bland inline bool IsNull()
        {
            return (this->GetPml2Address() == nullpaddr) && !this->GetPresent();
        }

        /*  Synchronization  */

        SPINLOCK(ContentLock, 10);
        SPINLOCK(PropertiesLock, 11);

    //private:

        uint64_t Value;
    };
    typedef Pml3Entry PdptEntry;

    /**
     *  Page Map Level 3: Page Directory Pointer Table.
     */
    struct Pml3
    {
        /*
         *  This table is indexed by bits 30-38 from the linear address.
         *  Naturally, bits 0-2 of the index are 0. (8-byte entries)
         */

        static const uint64_t AddressBits   = 0x0000007FC0000000ULL;
        static const uint64_t IndexOffset   = 30;

        /*  Constructors  */

        Pml3() = default;
        Pml3(Pml3 const &) = default;

        /*  Operators  */

        /**
         *  Gets the entrry at a given index.
         */
        __bland inline Pml3Entry & operator [](uint16_t const ind)
        {
            return this->Entries[ind];
        }

        /**
         *  Gets the entry corresponding to the given linear address.
         */
        __bland inline Pml3Entry & operator [](vaddrptr_t const vaddr)
        {
            //  Take the interesting bits from the linear address...
            //  Shift it by the required amount of bits...
            //  And use that as an index! :D
            return this->Entries[(vaddr.val & AddressBits) >> IndexOffset];
        }

        /*  Field(s)  */

    //private:

        Pml3Entry Entries[512]; //  Yeah...
    };
    typedef Pml3 Pdpt;


    /**
     *  Page Map Level 4 Entry
     */
    struct Pml4Entry
    {
        /*  Bit structure:
         *       0       : Present (if 1)
         *       1       : R/W (1 means writes allowed)
         *       2       : U/S (1 if usermode access allowed)
         *       3       : PWT (page-level write-through)
         *       4       : PCD (page-level cache disable)
         *       5       : Accessed
         *       6       : Ignored
         *       7       : Reserved (must be 0)
         *       8 -  11 : Ignored
         *      12 - M-1 : Physical address of PML3(PDPT) table; 4-KiB aligned.
         *       M -  51 : Reserved (must be 0)
         *      52 -  62 : Ignored
         *      63       : XD (eXecute Disable, if 1)
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1W( 0, Present)
        BITFIELD_DEFAULT_1W( 1, Writable)
        BITFIELD_DEFAULT_1W( 2, Userland)
        BITFIELD_DEFAULT_1W( 3, Pwt)
        BITFIELD_DEFAULT_1W( 4, Pcd)
        BITFIELD_DEFAULT_1W( 5, Accessed)
        BITFIELD_DEFAULT_1W(63, Xd)
        BITFIELD_DEFAULT_2W(12, 40, Pml3 * , Pml3Ptr)
        BITFIELD_DEFAULT_2W(12, 40, paddr_t, Pml3Address)

        /*  Constructors  */

        /**
         *  Creates an empty PML4 (PT) entry structure.
         */
        __bland inline Pml4Entry() : Value( 0ULL ) { }

        /**
         *  Creates a new PML4 entry structure that points to a PML3 table.
         */
        __bland inline Pml4Entry(const paddr_t pml3_paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    XD)
            : Value((pml3_paddr & Pml3PtrBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (XD             ? XdBit       : 0ULL))
        {

        }

        /**
         *  Creates a new PML4 entry structure that points to a PML3 table.
         */
        __bland inline Pml4Entry(const paddr_t pml3_paddr
                               , const bool    present
                               , const bool    writable
                               , const bool    userAccessible
                               , const bool    PWT
                               , const bool    PCD
                               , const bool    accessed
                               , const bool    XD)
            : Value((pml3_paddr & Pml3PtrBits)
                  | (present        ? PresentBit  : 0ULL)
                  | (writable       ? WritableBit : 0ULL)
                  | (userAccessible ? UserlandBit : 0ULL)
                  | (PWT            ? PwtBit      : 0ULL)
                  | (PCD            ? PcdBit      : 0ULL)
                  | (accessed       ? AccessedBit : 0ULL)
                  | (XD             ? XdBit       : 0ULL))
        {

        }

        /*  Properties  */

        __bland inline bool IsNull()
        {
            return (this->GetPml3Address() == nullpaddr) && !this->GetPresent();
        }

        /*  Synchronization  */

        SPINLOCK(ContentLock, 10);
        SPINLOCK(PropertiesLock, 11);

    //private:

        uint64_t Value;
    };

    /**
     *  Page Map Level 4
     */
    struct Pml4
    {
        /*
         *  This table is indexed by bits 39-47 from the linear address.
         *  Naturally, bits 0-2 of the index are 0. (8-byte entries)
         */

        static const uint64_t AddressBits   = 0x0000FF8000000000ULL;
        static const uint64_t IndexOffset   = 39;

        /*  Constructors  */

        Pml4() = default;
        Pml4(Pml4 const &) = default;

        /*  Operators  */

        /**
         *  Gets the entrry at a given index.
         */
        __bland inline Pml4Entry & operator [](uint16_t const ind)
        {
            return this->Entries[ind];
        }

        /**
         *  Gets the entry corresponding to the given linear address.
         */
        __bland inline Pml4Entry & operator [](vaddrptr_t const vaddr)
        {
            //  Take the interesting bits from the linear address...
            //  Shift it by the required amount of bits...
            //  And use that as an index! :D
            return this->Entries[(vaddr.val & AddressBits) >> IndexOffset];
        }

        /*  Field(s)  */

    //private:

        Pml4Entry Entries[512]; //  Yeah...
    };
}}
