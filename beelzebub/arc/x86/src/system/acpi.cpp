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

RsdpPtr     Acpi::RsdpPointer;
RsdtXsdtPtr Acpi::RsdtXsdtPointer;

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
    auto rsdp = Acpi::RsdpPointer;

    acpi_table_header * ptr;

    paddr_t const tableHeaderStart = (rsdp.GetVersion() == AcpiVersion::v1)
        ? (paddr_t)rsdp.GetVersion1()->RsdtPhysicalAddress
        : (paddr_t)rsdp.GetVersion2()->XsdtPhysicalAddress;

    paddr_t const tableHeaderEnd = tableHeaderStart + sizeof(acpi_table_header);

    paddr_t const tableStartPage     = RoundDown(tableHeaderStart, PageSize);
    paddr_t const tableHeaderEndPage = RoundUp  (tableHeaderEnd  , PageSize);
    psize_t const tableHeaderLength  = tableHeaderEndPage - tableStartPage;
    vaddr_t const vaddr = MemoryManagerAmd64::KernelHeapCursor.FetchAdd(tableHeaderLength);

    for (paddr_t offset = 0; offset < tableHeaderLength; offset += PageSize)
    {
        res = BootstrapMemoryManager.MapPage(vaddr + offset, tableStartPage + offset
                                           , PageFlags::Global, nullptr);

        assert_or(res.IsOkayResult()
            , "Failed to map page at %Xp (%XP) for %s: %H."
            , vaddr + offset, tableStartPage + offset
            , (rsdp.GetVersion() == AcpiVersion::v1) ? "RSDT" : "XSDT"
            , res)
        {
            return res;
        }
    }

    //Acpi::RsdtXsdtPointer = ptr;

    if (ptr == nullptr)
        return HandleResult::NotFound;
    else
        return HandleResult::Okay;
}
