#include <system/acpi.hpp>
#include <memory/manager_amd64.hpp>
#include <utils/checksum.hpp>
#include <string.h>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Utils;

/*****************
    ACPI class
*****************/

/*  Statics  */

RsdpPtr Acpi::RsdpPointer;

/*  Initialization  */

RsdpPtr Acpi::FindRsdp(uintptr_t const start, uintptr_t const end)
{
    RsdpPtr res {};

    for (uintptr_t location = RoundDown(start, 16); location < end; location += 16)
    {
        if (!memeq((void *)location, ACPI_SIG_RSDP, 8))
            continue;
        //  Doesn't start with the RSDP signature? SHOO!!1!

        if (0 != Checksum8((void *)location, 20))
            continue;
        //  Checksum fails? Odd.

        acpi_table_rsdp * const table = (acpi_table_rsdp *)location;

        res = RsdpPtr(table);
        res.SetVersion(AcpiVersion::v1);

        if (table->Revision >= 1)
        {
            //  Oh, version 2.0+? The table's larger and contains yet another
            //  checksum to check.

            if likely(0 == Checksum8((uint8_t *)location + 20, table->Length - 20))
            {
                //  The first 20 bytes already sum to 0, so there's no neeed to
                //  add them to the sum again...

                res.SetVersion(AcpiVersion::v2);
            }

            //  Oh, and if this checksum fails, perhaps a vendor messed up.
            //  It is assumed that a version 1 table is found.
        }

        break;
    }

    return Acpi::RsdpPointer = res;
}

RsdtXsdtPtr Acpi::FindRsdtXsdt(RsdpPtr const rsdp)
{
    RsdtXsdtPtr res {};

    acpi_table_header * ptr;

    paddr_t const tableHeaderStart = (rsdp.GetVersion() == AcpiVersion::v1)
        ? (paddr_t)rsdp.GetVersion1()->RsdtPhysicalAddress
        : (paddr_t)rsdp.GetVersion2()->XsdtPhysicalAddress;

    paddr_t const tableHeaderEnd = tableHeaderStart + sizeof(acpi_table_header);

    paddr_t const tableStartPage = RoundDown(tableHeaderStart, PageSize);
    paddr_t const tableEndPage   = RoundUp  (tableHeaderEnd  , PageSize);

    for (paddr_t paddr = tableStartPage; paddr < tableEndPage; paddr += PageSize)
    {

    }

    return Acpi::RsdtXsdtPointer = res;
}
