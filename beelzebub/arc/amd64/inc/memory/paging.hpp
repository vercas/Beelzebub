/**
 *  Bit 11 of all entries is used as a spinlock!
 */

#pragma once

#include <handles.h>

//  Creates a getter and setter for bit-based properties.
#define BITPROP(name)                                        \
__bland __forceinline bool MCATS2(Get, name)() const         \
{                                                            \
    return (this->Value & (1ULL << MCATS2(name, Bit)));      \
}                                                            \
__bland __forceinline void MCATS2(Set, name)(bool const val) \
{                                                            \
    if (val)                                                 \
        this->Value |=  (1ULL << MCATS2(name, Bit));         \
    else                                                     \
        this->Value &= ~(1ULL << MCATS2(name, Bit));         \
}

//  Creates the functions necessary for operating a spinlock over an entry.
#define SPINLOCK(name, bit)                                  \
__bland __forceinline bool MCATS2(TryAcquire, name)()        \
{                                                            \
    bool res = 0;                                            \
                                                             \
    asm volatile ("lock btsq $" #bit ", ([val]) \n\t"        \
                  "setnc [res] \n\t"                         \
                 : [res] "=r" (res)                          \
                 : [val] "m" (&this->Value)                  \
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
    asm volatile ("lock btrq $" #bit ", ([val]) \n\t"        \
                 :                                           \
                 : [val] "m" (&this->Value)                  \
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
		
        static const uint64_t PresentBit    =  0;
        static const uint64_t WritableBit   =  1;
        static const uint64_t UserlandBit   =  2;
        static const uint64_t PwtBit        =  3;
        static const uint64_t PcdBit        =  4;
        static const uint64_t AccessedBit   =  5;
        static const uint64_t DirtyBit      =  6;
        static const uint64_t PatBit        =  7;
        static const uint64_t GlobalBit     =  8;
        static const uint64_t XdBit         = 63;

        static const uint64_t AddressBits   = 0x000FFFFFFFFFF000ULL;

        /*  Constructors  */

        /**
         *  Creates an empty PML1 (PT) entry structure.
         */
        __bland __forceinline Pml1Entry() : Value( 0ULL ) { }

        /**
         *  Creates a new PML1 (PT) entry structure that maps a 4-KiB page.
         */
        __bland __forceinline Pml1Entry(const paddr_t paddr
                                      , const bool    present
                                      , const bool    writable
                                      , const bool    userAccessible
                                      , const bool    global
                                      , const bool    XD)
        {
            this->Value = (paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (global         ? (1ULL << GlobalBit)   : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL);
        }

        /**
         *  Creates a new PML1 (PT) entry structure that maps a 4-KiB page.
         */
        __bland __forceinline Pml1Entry(const paddr_t paddr
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
        {
            this->Value = (paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (PWT            ? (1ULL << PwtBit)      : 0ULL)
                        | (PCD            ? (1ULL << PcdBit)      : 0ULL)
                        | (accessed       ? (1ULL << AccessedBit) : 0ULL)
                        | (dirty          ? (1ULL << DirtyBit)    : 0ULL)
                        | (PAT            ? (1ULL << PatBit)      : 0ULL)
                        | (global         ? (1ULL << GlobalBit)   : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL);
        }

        /*  Properties  */

        BITPROP(Present)
        BITPROP(Writable)
        BITPROP(Userland)
        BITPROP(Pwt)
        BITPROP(Pcd)
        BITPROP(Accessed)
        BITPROP(Dirty)
        BITPROP(Pat)
        BITPROP(Global)
        BITPROP(Xd)

        /**
         *  Gets the physical address of the 4-KiB page.
         */
        __bland __forceinline paddr_t GetAddress() const
        {
            return (paddr_t)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the 4-KiB page.
         */
        __bland __forceinline void SetAddress(const paddr_t paddr)
        {
            this->Value = (paddr       &  AddressBits)
                        | (this->Value & ~AddressBits);
        }

        __bland __forceinline bool IsNull()
        {
            return (this->GetAddress() == (paddr_t)0) && !this->GetPresent();
        }

        /*  Synchronization  */

        SPINLOCK(ContentLock, 10);
        SPINLOCK(PropertiesLock, 11);

        /*  Operators  */

        /**
         *  Gets the value of a bit.
         */
        __bland __forceinline bool operator [](const uint8_t bit) const
        {
            return 0 != (this->Value & (1 << bit));
        }

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
        Pml1(Pml1 const&) = default;

        /*  Operators  */

        /**
         *  Gets the entrry at a given index.
         */
        __bland __forceinline Pml1Entry & operator [](const uint16_t ind)
        {
            return this->Entries[ind];
        }

        /**
         *  Gets the entry corresponding to the given linear address.
         */
        __bland __forceinline Pml1Entry & operator [](const vaddrptr_t vaddr)
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

        static const uint64_t PresentBit    =  0;
        static const uint64_t WritableBit   =  1;
        static const uint64_t UserlandBit   =  2;
        static const uint64_t PwtBit        =  3;
        static const uint64_t PcdBit        =  4;
        static const uint64_t AccessedBit   =  5;
        static const uint64_t DirtyBit      =  6;
        static const uint64_t PageSizeBit   =  7;
        static const uint64_t GlobalBit     =  8;
        static const uint64_t PatBit        = 12;
        static const uint64_t XdBit         = 63;

        static const uint64_t AddressBits   = 0x000FFFFFFFFFF000ULL;

        /*  Constructors  */

        /**
         *  Creates an empty PML2 (PD) entry structure.
         */
        __bland __forceinline Pml2Entry() : Value( 0ULL ) { }

        /**
         *  Creates a new PML2 (PD) entry structure that points to a PML1 (PT) table.
         */
        __bland __forceinline Pml2Entry(const paddr_t pml1_paddr
                                      , const bool    present
                                      , const bool    writable
                                      , const bool    userAccessible
                                      , const bool    XD)
        {
            this->Value = (pml1_paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL);
        }

        /**
         *  Creates a new PML2 (PD) entry structure that points to a PML1 (PT) table.
         */
        __bland __forceinline Pml2Entry(const paddr_t pml1_paddr
                                      , const bool    present
                                      , const bool    writable
                                      , const bool    userAccessible
                                      , const bool    PWT
                                      , const bool    PCD
                                      , const bool    accessed
                                      , const bool    XD)
        {
            this->Value = (pml1_paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (PWT            ? (1ULL << PwtBit)      : 0ULL)
                        | (PCD            ? (1ULL << PcdBit)      : 0ULL)
                        | (accessed       ? (1ULL << AccessedBit) : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL);
        }

        /**
         *  Creates a new PML2 (PD) entry structure that maps a 2-MiB page.
         */
        __bland __forceinline Pml2Entry(const paddr_t paddr
                                      , const bool    present
                                      , const bool    writable
                                      , const bool    userAccessible
                                      , const bool    global
                                      , const bool    XD)
        {
            this->Value = ((uint64_t)paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (global         ? (1ULL << GlobalBit)   : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL)
                        |                   PageSizeBit        ;
        }

        /**
         *  Creates a new PML2 (PD) entry structure that maps a 2-MiB page.
         */
        __bland __forceinline Pml2Entry(const paddr_t paddr
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
        {
            this->Value = ((uint64_t)paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (PWT            ? (1ULL << PwtBit)      : 0ULL)
                        | (PCD            ? (1ULL << PcdBit)      : 0ULL)
                        | (accessed       ? (1ULL << AccessedBit) : 0ULL)
                        | (dirty          ? (1ULL << DirtyBit)    : 0ULL)
                        | (global         ? (1ULL << GlobalBit)   : 0ULL)
                        | (PAT            ? (1ULL << PatBit)      : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL)
                        |                   PageSizeBit        ;
        }

        /*  Properties  */

        BITPROP(Present)
        BITPROP(Writable)
        BITPROP(Userland)
        BITPROP(Pwt)
        BITPROP(Pcd)
        BITPROP(Accessed)
        BITPROP(Dirty)
        BITPROP(PageSize)
        BITPROP(Global)
        BITPROP(Pat)
        BITPROP(Xd)

        /**
         *  Gets the physical address of the PML1 (PT) table.
         */
        __bland __forceinline Pml1 * GetPml1Ptr() const
        {
            return (Pml1 *)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the PML1 (PT) table.
         */
        __bland __forceinline void SetPml1Ptr(const paddr_t paddr)
        {
            this->Value = (paddr       &  AddressBits)
                        | (this->Value & ~AddressBits);
        }

        /**
         *  Gets the physical address of the 2-MiB page.
         */
        __bland __forceinline paddr_t GetAddress() const
        {
            return (paddr_t)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the 2-MiB page.
         */
        __bland __forceinline void SetAddress(const paddr_t paddr)
        {
            this->Value = (paddr       &  AddressBits)
                        | (this->Value & ~AddressBits);
        }

        __bland __forceinline bool IsNull()
        {
            return (this->GetAddress() == (paddr_t)0) && !this->GetPresent();
        }

        /*  Synchronization  */

        SPINLOCK(ContentLock, 10);
        SPINLOCK(PropertiesLock, 11);

        /*  Operators  */

        /**
         *  Gets the value of a bit.
         */
        __bland __forceinline bool operator [](const uint8_t bit) const
        {
            return 0 != (this->Value & (1 << bit));
        }

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
        Pml2(Pml2 const&) = default;

        /*  Operators  */

        /**
         *  Gets the entrry at a given index.
         */
        __bland __forceinline Pml2Entry & operator [](const uint16_t ind)
        {
            return this->Entries[ind];
        }

        /**
         *  Gets the entry corresponding to the given linear address.
         */
        __bland __forceinline Pml2Entry & operator [](const vaddrptr_t vaddr)
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

        static const uint64_t PresentBit    =  0;
        static const uint64_t WritableBit   =  1;
        static const uint64_t UserlandBit   =  2;
        static const uint64_t PwtBit        =  3;
        static const uint64_t PcdBit        =  4;
        static const uint64_t AccessedBit   =  5;
        static const uint64_t DirtyBit      =  6;
        static const uint64_t PageSizeBit   =  7;
        static const uint64_t GlobalBit     =  8;
        static const uint64_t PatBit        = 12;
        static const uint64_t XdBit         = 63;

        static const uint64_t AddressBits   = 0x000FFFFFFFFFF000ULL;

        /*  Constructors  */

        /**
         *  Creates an empty PML3 (PDPT) entry structure.
         */
        __bland __forceinline Pml3Entry() : Value( 0ULL ) { }

        /**
         *  Creates a new PML3 entry structure that points to a PML2 table.
         */
        __bland __forceinline Pml3Entry(const paddr_t pml2_paddr
                                      , const bool    present
                                      , const bool    writable
                                      , const bool    userAccessible
                                      , const bool    XD)
        {
            this->Value = (pml2_paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL);
        }

        /**
         *  Creates a new PML3 entry structure that points to a PML2 table.
         */
        __bland __forceinline Pml3Entry(const paddr_t pml2_paddr
                                      , const bool    present
                                      , const bool    writable
                                      , const bool    userAccessible
                                      , const bool    PWT
                                      , const bool    PCD
                                      , const bool    accessed
                                      , const bool    XD)
        {
            this->Value = (pml2_paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (PWT            ? (1ULL << PwtBit)      : 0ULL)
                        | (PCD            ? (1ULL << PcdBit)      : 0ULL)
                        | (accessed       ? (1ULL << AccessedBit) : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL);
        }

        /**
         *  Creates a new PML3 entry structure that points to a 1-GiB page.
         */
        __bland __forceinline Pml3Entry(const paddr_t paddr
                                      , const bool    present
                                      , const bool    writable
                                      , const bool    userAccessible
                                      , const bool    global
                                      , const bool    XD)
        {
            this->Value = (paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (global         ? (1ULL << GlobalBit)   : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL)
                        |                   PageSizeBit        ;
        }

        /**
         *  Creates a new PML3 entry structure that points to a 1-GiB page.
         */
        __bland __forceinline Pml3Entry(const paddr_t paddr
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
        {
            this->Value = (paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (PWT            ? (1ULL << PwtBit)      : 0ULL)
                        | (PCD            ? (1ULL << PcdBit)      : 0ULL)
                        | (accessed       ? (1ULL << AccessedBit) : 0ULL)
                        | (dirty          ? (1ULL << DirtyBit)    : 0ULL)
                        | (global         ? (1ULL << GlobalBit)   : 0ULL)
                        | (PAT            ? (1ULL << PatBit)      : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL)
                        |                   PageSizeBit        ;
        }

        /*  Properties  */

        BITPROP(Present)
        BITPROP(Writable)
        BITPROP(Userland)
        BITPROP(Pwt)
        BITPROP(Pcd)
        BITPROP(Accessed)
        BITPROP(Dirty)
        BITPROP(PageSize)
        BITPROP(Global)
        BITPROP(Pat)
        BITPROP(Xd)

        /**
         *  Gets the physical address of the PML2 (PD) table.
         */
        __bland __forceinline Pml2 * GetPml2Ptr() const
        {
            return (Pml2 *)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the PML2 (PD) table.
         */
        __bland __forceinline void SetPml2Ptr(const paddr_t paddr)
        {
            this->Value = (paddr       &  AddressBits)
                        | (this->Value & ~AddressBits);
        }

        /**
         *  Gets the physical address of the 1-GiB page.
         */
        __bland __forceinline paddr_t GetAddress() const
        {
            return (paddr_t)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the 1-GiB page.
         */
        __bland __forceinline void SetAddress(const paddr_t paddr)
        {
            this->Value = (paddr       &  AddressBits)
                        | (this->Value & ~AddressBits);
        }

        __bland __forceinline bool IsNull()
        {
            return (this->GetAddress() == (paddr_t)0) && !this->GetPresent();
        }

        /*  Synchronization  */

        SPINLOCK(ContentLock, 10);
        SPINLOCK(PropertiesLock, 11);

        /*  Operators  */

        /**
         *  Gets the value of a bit.
         */
        __bland __forceinline bool operator [](const uint8_t bit) const
        {
            return 0 != (this->Value & (1 << bit));
        }

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
        Pml3(Pml3 const&) = default;

        /*  Operators  */

        /**
         *  Gets the entrry at a given index.
         */
        __bland __forceinline Pml3Entry & operator [](const uint16_t ind)
        {
            return this->Entries[ind];
        }

        /**
         *  Gets the entry corresponding to the given linear address.
         */
        __bland __forceinline Pml3Entry & operator [](const vaddrptr_t vaddr)
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

        static const uint64_t PresentBit    =  0;
        static const uint64_t WritableBit   =  1;
        static const uint64_t UserlandBit   =  2;
        static const uint64_t PwtBit        =  3;
        static const uint64_t PcdBit        =  4;
        static const uint64_t AccessedBit   =  5;
        static const uint64_t XdBit         = 63;

        static const uint64_t AddressBits = 0x000FFFFFFFFFF000ULL;

        /*  Constructors  */

        /**
         *  Creates an empty PML4 (PT) entry structure.
         */
        __bland __forceinline Pml4Entry() : Value( 0ULL ) { }

        /**
         *  Creates a new PML4 entry structure that points to a PML3 table.
         */
        __bland __forceinline Pml4Entry(const paddr_t pml3_paddr
                                      , const bool    present
                                      , const bool    writable
                                      , const bool    userAccessible
                                      , const bool    XD)
        {
            this->Value = (pml3_paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL);
        }

        /**
         *  Creates a new PML4 entry structure that points to a PML3 table.
         */
        __bland __forceinline Pml4Entry(const paddr_t pml3_paddr
                                      , const bool    present
                                      , const bool    writable
                                      , const bool    userAccessible
                                      , const bool    PWT
                                      , const bool    PCD
                                      , const bool    accessed
                                      , const bool    XD)
        {
            this->Value = (pml3_paddr & AddressBits)
                        | (present        ? (1ULL << PresentBit)  : 0ULL)
                        | (writable       ? (1ULL << WritableBit) : 0ULL)
                        | (userAccessible ? (1ULL << UserlandBit) : 0ULL)
                        | (PWT            ? (1ULL << PwtBit)      : 0ULL)
                        | (PCD            ? (1ULL << PcdBit)      : 0ULL)
                        | (accessed       ? (1ULL << AccessedBit) : 0ULL)
                        | (XD             ? (1ULL << XdBit)       : 0ULL);
        }

        /*  Properties  */

        BITPROP(Present)
        BITPROP(Writable)
        BITPROP(Userland)
        BITPROP(Pwt)
        BITPROP(Pcd)
        BITPROP(Accessed)
        BITPROP(Xd)

        /**
         *  Gets the physical address of the PML3 (PDPT) table.
         */
        __bland __forceinline Pml3 * GetPml3Ptr() const
        {
            return (Pml3 *)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the PML3 (PDPT) table.
         */
        __bland __forceinline void SetPml3Ptr(const paddr_t paddr)
        {
            this->Value = (paddr       &  AddressBits)
                        | (this->Value & ~AddressBits);
        }

        /**
         *  Gets the physical address of the PML3 (PDPT) page.
         */
        __bland __forceinline paddr_t GetAddress() const
        {
            return (paddr_t)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the PML3 (PDPT) page.
         */
        __bland __forceinline void SetAddress(const paddr_t paddr)
        {
            this->Value = (paddr       &  AddressBits)
                        | (this->Value & ~AddressBits);
        }

        __bland __forceinline bool IsNull()
        {
            return (this->GetAddress() == (paddr_t)0) && !this->GetPresent();
        }

        /*  Synchronization  */

        SPINLOCK(ContentLock, 10);
        SPINLOCK(PropertiesLock, 11);

        /*  Operators  */

        /**
         *  Gets the value of a bit.
         */
        __bland __forceinline bool operator [](const uint8_t bit) const
        {
            return 0 != (this->Value & (1 << bit));
        }

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
        Pml4(Pml4 const&) = default;

        /*  Operators  */

        /**
         *  Gets the entrry at a given index.
         */
        __bland __forceinline Pml4Entry & operator [](const uint16_t ind)
        {
            return this->Entries[ind];
        }

        /**
         *  Gets the entry corresponding to the given linear address.
         */
        __bland __forceinline Pml4Entry & operator [](const vaddrptr_t vaddr)
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
