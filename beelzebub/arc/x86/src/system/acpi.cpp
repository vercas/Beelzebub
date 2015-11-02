#include <system/acpi.hpp>
#include <entry.h>

#include <utils/checksum.hpp>
#include <string.h>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;
using namespace Beelzebub::Utils;

/*****************
    ACPI class
*****************/

/*  Statics  */

RsdpPtr           Acpi::RsdpPointer;
acpi_table_rsdt * Acpi::RsdtPointer = nullptr;
acpi_table_xsdt * Acpi::XsdtPointer = nullptr;

/*  Initialization  */

Handle Acpi::FindRsdp(uintptr_t const start, uintptr_t const end)
{
    RsdpPtr ptr {};

    for (uintptr_t location = RoundDown(start, 16); location < end; location += 16)
    {
        if (!memeq((void *)location, ACPI_SIG_RSDP, 8))
            continue;
        //  Doesn't start with the RSDP signature? SHOO!!1!

        if (0 != Checksum8((void *)location, 20))
            continue;
        //  Checksum fails? Odd.

        acpi_table_rsdp * const table = (acpi_table_rsdp *)location;

        ptr = RsdpPtr(table);
        ptr.SetVersion(AcpiVersion::v1);

        if (table->Revision >= 1)
        {
            //  Oh, version 2.0+? The table's larger and contains yet another
            //  checksum to check.

            if likely(0 == Checksum8((uint8_t *)location + 20, table->Length - 20))
            {
                //  The first 20 bytes already sum to 0, so there's no neeed to
                //  add them to the sum again...

                ptr.SetVersion(AcpiVersion::v2);
            }

            //  Oh, and if this checksum fails, perhaps a vendor messed up.
            //  It is assumed that a version 1 table is found.
        }

        break;
    }

    Acpi::RsdpPointer = ptr;

    if (ptr == nullptr)
        return HandleResult::NotFound;
    else
        return HandleResult::Okay;
}

Handle Acpi::FindRsdtXsdt()
{
    Handle res;
    auto rsdp = RsdpPointer;

    //  So, first we attempt to find and parse the XSDT, if any.
    //  If that fails or there is no XSDT, it tries the RSDT.

    if likely(rsdp.GetVersion() == AcpiVersion::v2)
    {
        //  Why likely? So GCC doesn't attempt to initialize the locals beyond
        //  this if statement prior to executing it.

        vaddr_t xsdtVaddr = nullvaddr;
        res = Acpi::MapTable((paddr_t)rsdp.GetVersion2()->XsdtPhysicalAddress, xsdtVaddr);

        assert_or(res.IsOkayResult()
            , "Failed to map XSDT: %H%n"
            , res)
        {
            return res;
        }

        XsdtPointer = (acpi_table_xsdt *)xsdtVaddr;

        uint8_t sumXsdt = Checksum8(XsdtPointer, XsdtPointer->Header.Length);

        if (0 == sumXsdt)
            return HandleResult::Okay;

        assert(sumXsdt == 0
            , "XSDT checksum failed: %u1%n"
            , sumXsdt);

        //  If the checksum fails in release mode... Maybe we try the RSDT?
    }

    paddr_t const rsdtHeaderStart = (paddr_t)((acpi_rsdp_common *)rsdp.GetInvariantValue())->RsdtPhysicalAddress;

    ASSERT(rsdtHeaderStart != nullpaddr
        , "RSDT physical address seems to be null!");
    //  Cawme awn!

    vaddr_t rsdtVaddr = nullvaddr;
    res = Acpi::MapTable(rsdtHeaderStart, rsdtVaddr);

    assert_or(res.IsOkayResult()
        , "Failed to map RSDT: %H%n"
        , res)
    {
        return res;
    }

    RsdtPointer = (acpi_table_rsdt *)rsdtVaddr;

    if (rsdtVaddr == nullvaddr)
        return HandleResult::NotFound;

    uint8_t sumRsdt = Checksum8(RsdtPointer, RsdtPointer->Header.Length);

    assert_or(0 == sumRsdt
        , "RSDT checksum failed: %u1%n"
        , sumRsdt)
    {
        return HandleResult::IntegrityFailure;
    }

    //  If this checksum fails... There's nothing else to try. :(
    
    return HandleResult::Okay;
}

/*  Utilities  */

Handle Acpi::MapTable(paddr_t const header, vaddr_t & ptr)
{
    Handle res;

    //  First the table headers and the fields that we are sure to find.

    paddr_t const tabHeaderEnd = header + sizeof(acpi_table_header);

    paddr_t const tabStartPage     = RoundDown(header      , PageSize);
    paddr_t const tabHeaderEndPage = RoundUp  (tabHeaderEnd, PageSize);
    vaddr_t const vaddr = MemoryManagerAmd64::KernelHeapCursor.FetchAdd(tabHeaderEndPage - tabStartPage);
    paddr_t offset1 = 0;

    for (/* nothing */; tabStartPage + offset1 < tabHeaderEndPage; offset1 += PageSize)
    {
        res = BootstrapMemoryManager.MapPage(vaddr + offset1
                                           , tabStartPage + offset1
                                           , PageFlags::Global, nullptr);

        assert_or(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP) for XSDT header: %H%n"
            , vaddr + offset1, tabStartPage + offset1
            , res)
        {
            return res;
        }
    }

    //  Then the rest of the table.

    ptr = (vaddr_t)(vaddr + (header - tabStartPage));
    auto const tabPtr = (acpi_table_header *)(uintptr_t)ptr;

    paddr_t const tabEnd = header + tabPtr->Length;
    paddr_t const tabEndPage = RoundUp(tabEnd, PageSize);
    vaddr_t const vaddrExtra = MemoryManagerAmd64::KernelHeapCursor.FetchAdd(tabEndPage - tabHeaderEndPage);

    assert(vaddrExtra - vaddr == tabHeaderEndPage - tabStartPage
        , "Discrepancy observed while mapping XSDT - virtual addresses are "
          "not contiguous: %Xp-%Xp + %Xp-...%n"
        , vaddr, vaddr + tabHeaderEndPage - tabStartPage, vaddrExtra);

    for (/* nothing */; tabStartPage + offset1 < tabEndPage; offset1 += PageSize)
    {
        res = BootstrapMemoryManager.MapPage(vaddr + offset1
                                           , tabStartPage + offset1
                                           , PageFlags::Global, nullptr);

        assert_or(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP) for XSDT body: %H%n"
            , vaddr + offset1, tabStartPage + offset1
            , res)
        {
            return res;
        }
    }

    return HandleResult::Okay;
}
