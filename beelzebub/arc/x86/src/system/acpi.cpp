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

/*  For internal use  */

paddr_t                     MadtPaddr = nullpaddr;
SystemDescriptorTableSource MadtSrc   = SystemDescriptorTableSource::None;

paddr_t                     SratPaddr = nullpaddr;
SystemDescriptorTableSource SratSrc   = SystemDescriptorTableSource::None;

/*****************
    ACPI class
*****************/

/*  Statics  */

RsdpPtr           Acpi::RsdpPointer;
acpi_table_rsdt * Acpi::RsdtPointer = nullptr;
acpi_table_xsdt * Acpi::XsdtPointer = nullptr;
acpi_table_madt * Acpi::MadtPointer = nullptr;
acpi_table_srat * Acpi::SratPointer = nullptr;

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

        assert_or(memeq(XsdtPointer->Header.Signature, ACPI_SIG_XSDT, ACPI_NAME_SIZE)
            , "XSDT signature doesn't appear to be valid..? (\"%S\")%n"
            , ACPI_NAME_SIZE, XsdtPointer->Header.Signature)
        {
            XsdtPointer = nullptr;

            goto find_rsdt;

            //  Under these circumstances, the next statement cannot be allowed
            //  to execute because it will result in a page fault when
            //  attempting to access a null pointer.
        }

        uint8_t sumXsdt = Checksum8(XsdtPointer, XsdtPointer->Header.Length);

        assert_or(0 == sumXsdt
            , "XSDT checksum failed: %u1%n"
            , sumXsdt)
        {
            XsdtPointer = nullptr;
        }

        //  If the checksum fails in release mode... Maybe we try the RSDT?
        //  Nulling the XSDT pointer indicates that it's either absent or
        //  invalid. Both cases would be rather weird, but adaptation means
        //  survival.
    }

find_rsdt:

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

    assert_or(memeq(RsdtPointer->Header.Signature, ACPI_SIG_RSDT, ACPI_NAME_SIZE)
        , "RSDT signature doesn't appear to be valid..? (\"%S\")%n"
        , ACPI_NAME_SIZE, RsdtPointer->Header.Signature)
    {
        if (XsdtPointer == nullptr)
            return HandleResult::IntegrityFailure;
        else
            return HandleResult::Okay;

        //  Signature invalid? Try to return with a valid XSDT at least.
    }

    uint8_t sumRsdt = Checksum8(RsdtPointer, RsdtPointer->Header.Length);

    assert_or(0 == sumRsdt
        , "RSDT checksum failed: %u1%n"
        , sumRsdt)
    {
        if (XsdtPointer == nullptr)
            return HandleResult::IntegrityFailure;
        else
            return HandleResult::Okay;

        //  Okay, so maybe this checksum fails, but all is not lost if the XSDT
        //  is valid.
    }
    
    return HandleResult::Okay;
}

Handle Acpi::FindSystemDescriptorTables()
{
    Handle res;

    size_t totalEntries = 0;

    if (XsdtPointer != nullptr)
    {
        size_t entryCount = (XsdtPointer->Header.Length - sizeof(acpi_table_header)) / 8;

        for (size_t i = 0; i < entryCount; ++i)
        {
            res = HandleSystemDescriptorTable((paddr_t)XsdtPointer->TableOffsetEntry[i]
                                            , SystemDescriptorTableSource::Xsdt);

            if (res.IsResult(HandleResult::CardinalityViolation))
            {
                MSG("Found duplicate table in XSDT!");

                res = HandleResult::Okay;

                //  Yes, we let the user know if possible and move on.
            }
            else
            {
                assert_or(res.IsOkayResult()
                    , "Failed to handle a system descriptor table from the XSDT: %H%n"
                    , res)
                {
                    return res;
                }
            }

            ++totalEntries;
        }
    }

    if (RsdtPointer != nullptr)
    {
        size_t entryCount = (RsdtPointer->Header.Length - sizeof(acpi_table_header)) / 4;

        for (size_t i = 0; i < entryCount; ++i)
        {
            res = HandleSystemDescriptorTable((paddr_t)RsdtPointer->TableOffsetEntry[i]
                                            , SystemDescriptorTableSource::Rsdt);

            if (res.IsResult(HandleResult::CardinalityViolation))
            {
                MSG("Found duplicate table in RSDT!");

                res = HandleResult::Okay;
            }
            else
            {
                assert_or(res.IsOkayResult()
                    , "Failed to handle a system descriptor table from the RSDT: %H%n"
                    , res)
                {
                    return res;
                }
            }

            ++totalEntries;
        }
    }

    if (totalEntries > 0)
        return HandleResult::Okay;
    else
        return HandleResult::NotFound;
    //  Well, no tables were found? :C
}

/*  Specific table handling  */

Handle Acpi::HandleSystemDescriptorTable(paddr_t const paddr, SystemDescriptorTableSource const src)
{
    Handle res;

    vaddr_t vaddr = nullvaddr;
    res = Acpi::MapTable(paddr, vaddr);

    assert_or(res.IsOkayResult()
        , "Failed to map a system descriptor table: %H%n"
        , res)
    {
        return res;
    }

    auto headerPtr = (acpi_table_header *)vaddr;

    uint8_t sumXsdt = Checksum8(headerPtr, headerPtr->Length);

    assert_or(0 == sumXsdt
        , "System descriptor table checksum failed: %u1%n"
        , sumXsdt)
    {
        headerPtr = nullptr;

        return HandleResult::IntegrityFailure;
    }

    msg("~[ FOUND TABLE \"%S\" @ %Xp (%XP) in %s ]~%n"
        , ACPI_NAME_SIZE, headerPtr->Signature, vaddr, paddr
        , (src == SystemDescriptorTableSource::Xsdt) ? ACPI_SIG_XSDT : ACPI_SIG_RSDT);

    if (memeq(headerPtr->Signature, ACPI_SIG_MADT, ACPI_NAME_SIZE))
        return Acpi::HandleMadt(vaddr, paddr, src);
    else if (memeq(headerPtr->Signature, ACPI_SIG_SRAT, ACPI_NAME_SIZE))
        return Acpi::HandleSrat(vaddr, paddr, src);

    return HandleResult::Okay;
}

Handle Acpi::HandleMadt(vaddr_t const vaddr, paddr_t const paddr, SystemDescriptorTableSource const src)
{
    if (MadtPaddr == paddr || MadtSrc != src)
        return HandleResult::Okay;
    //  Same physical address or different source table? No problemo, then.

    assert_or(MadtPointer == nullptr
        , "Duplicate (different) MADTs found under the same table (%s)?!%n"
          "First @ %Xp (%XP);%n"
          "Second @ %Xp (%XP)."
        , (src == SystemDescriptorTableSource::Xsdt) ? ACPI_SIG_XSDT : ACPI_SIG_RSDT
        , MadtPointer, MadtPaddr, vaddr, paddr)
    {
        return HandleResult::CardinalityViolation;
    }

    MadtPointer = (acpi_table_madt *)vaddr;
    MadtPaddr = paddr;
    MadtSrc = src;

    return HandleResult::Okay;
}

Handle Acpi::HandleSrat(vaddr_t const vaddr, paddr_t const paddr, SystemDescriptorTableSource const src)
{
    if (SratPaddr == paddr || SratSrc != src)
        return HandleResult::Okay;
    //  Same physical address or different source table? No problemo, then.

    assert_or(SratPointer == nullptr
        , "Duplicate (different) SRATs found under the same table (%s)?!%n"
          "First @ %Xp (%XP);%n"
          "Second @ %Xp (%XP)."
        , (src == SystemDescriptorTableSource::Xsdt) ? ACPI_SIG_XSDT : ACPI_SIG_RSDT
        , SratPointer, SratPaddr, vaddr, paddr)
    {
        return HandleResult::CardinalityViolation;
    }

    SratPointer = (acpi_table_srat *)vaddr;
    SratPaddr = paddr;
    SratSrc = src;

    return HandleResult::Okay;
}

/*  Utilities  */

Handle Acpi::MapTable(paddr_t const header, vaddr_t & ptr)
{
    if unlikely(header == nullpaddr)
    {
        ptr = nullvaddr;

        return HandleResult::ArgumentNull;
    }

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
            , "Failed to map page at %Xp (%XP) for table header: %H%n"
            , vaddr + offset1, tabStartPage + offset1
            , res)
        {
            ptr = nullvaddr;

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
        , "Discrepancy observed while mapping table - virtual addresses are "
          "not contiguous: %Xp-%Xp + %Xp-...%n"
        , vaddr, vaddr + tabHeaderEndPage - tabStartPage, vaddrExtra);

    for (/* nothing */; tabStartPage + offset1 < tabEndPage; offset1 += PageSize)
    {
        res = BootstrapMemoryManager.MapPage(vaddr + offset1
                                           , tabStartPage + offset1
                                           , PageFlags::Global, nullptr);

        assert_or(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP) for table body: %H%n"
            , vaddr + offset1, tabStartPage + offset1
            , res)
        {
            ptr = nullvaddr;

            return res;
        }
    }

    return HandleResult::Okay;
}
