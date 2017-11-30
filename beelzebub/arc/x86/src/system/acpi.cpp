/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#include "system/acpi.hpp"
#include "memory/vmm.hpp"
#include "memory/vmm.arc.hpp"
#include "entry.h"

#include <beel/utils/checksums.hpp>
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

size_t Acpi::LapicCount = 0;
size_t Acpi::PresentLapicCount = 0;
size_t Acpi::IoapicCount = 0;

paddr_t Acpi::RangeBottom = nullvaddr;
paddr_t Acpi::RangeTop = nullvaddr;
vaddr_t Acpi::VirtualBase = nullvaddr;

/*  Initialization  */

Handle Acpi::Crawl()
{
    Handle res;

    res = FindRsdp();

    assert_or(res.IsOkayResult()
        , "Unable to find RSDP! %H%n"
        , res)
    {
        return res;
    }

    res = FindRsdtXsdt();

    assert_or(res.IsOkayResult() && (Acpi::RsdtPointer != nullptr || Acpi::XsdtPointer != nullptr)
        , "Unable to find a valid RSDT or XSDT!%n"
          "RSDT @ %Xp; XSDT @ %X;%n"
          "Result = %H"
        , Acpi::RsdtPointer, Acpi::XsdtPointer, res)
    {
        return res;
    }

    res = FindSystemDescriptorTables();

    assert_or(res.IsOkayResult()
        , "Failure finding system descriptor tables!%n"
          "Result = %H"
        , res)
    {
        return res;
    }

    return res;
}

Handle Acpi::Remap()
{
    if unlikely(VirtualBase != 0)
        return HandleResult::CardinalityViolation;

    RangeBottom = RoundDown(RangeBottom, PageSize);
    RangeTop = RoundUp(RangeTop, PageSize);

    vsize_t const size { RangeTop - RangeBottom };

    Handle res = Vmm::AllocatePages(nullptr
        , size
        , MemoryAllocationOptions::Reserve | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global
        , MemoryContent::AcpiTable
        , VirtualBase);

    assert_or(res.IsOkayResult()
        , "Failed to reserve %Xs bytes of kernel heap space for ACPI tables: %H."
        , size, res)
    {
        VirtualBase = 0;

        return res;
    }

    res = Vmm::MapRange(nullptr
        , VirtualBase
        , RangeBottom
        , size
        , MemoryFlags::Global
        , MemoryMapOptions::NoReferenceCounting);

    assert_or(res.IsOkayResult()
        , "Failed to map range at %Xp to %XP (%Xs bytes) for ACPI tables: %H."
        , VirtualBase, RangeBottom, size
        , res)
    {
        VirtualBase = 0;

        return res;
    }

    RsdpPointer = RsdpPtr(reinterpret_cast<acpi_table_rsdp *>(reinterpret_cast<uintptr_t>(RsdpPointer.GetInvariantValue()) + VmmArc::IsaDmaStart.Value));

    #define REMAP(ptr) \
    ptr = reinterpret_cast<decltype(ptr)>(reinterpret_cast<uintptr_t>(ptr) - RangeBottom + VirtualBase);

    REMAP(RsdtPointer)
    REMAP(XsdtPointer)
    REMAP(MadtPointer)
    REMAP(SratPointer)

    #undef REMAP

    return HandleResult::Okay;
}

/*  Specific table handling  */

Handle Acpi::FindRsdp()
{
    RsdpPtr ptr {};

    for (uintptr_t location = RoundDown(RsdpStart, 16); location < RsdpEnd; location += 16)
    {
        if (!::memeq((void *)location, ACPI_SIG_RSDP, 8))
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
    auto const rsdp = RsdpPointer;

    //  So, first it attempts to find and parse the XSDT, if any.
    //  If that fails or there is no XSDT, it tries the RSDT.

    if likely(rsdp.GetVersion() == AcpiVersion::v2)
    {
        //  Why likely? So GCC doesn't attempt to initialize the locals beyond
        //  this if statement prior to executing it.

        XsdtPointer = (acpi_table_xsdt *)rsdp.GetVersion2()->XsdtPhysicalAddress;

        assert_or(::memeq(XsdtPointer->Header.Signature, ACPI_SIG_XSDT, ACPI_NAME_SIZE)
            , "XSDT signature doesn't appear to be valid..? (\"%S\")%n"
            , ACPI_NAME_SIZE, XsdtPointer->Header.Signature)
        {
            XsdtPointer = nullptr;

            goto find_rsdt;

            //  Under these circumstances, the next statement cannot be allowed
            //  to execute because it will result in a page fault when
            //  attempting to access a null pointer.
        }

        uint8_t const sumXsdt = Checksum8(XsdtPointer, XsdtPointer->Header.Length);

        assert_or(0 == sumXsdt
            , "XSDT checksum failed: %u1%n"
            , sumXsdt)
        {
            XsdtPointer = nullptr;

            goto find_rsdt;
        }

        //  If the checksum fails in release mode... Maybe try the RSDT?
        //  Nulling the XSDT pointer indicates that it's either absent or
        //  invalid. Both cases would be rather weird, but adaptation means
        //  survival.

        ConsiderAddress(XsdtPointer);
        ConsiderAddress((uintptr_t)XsdtPointer + XsdtPointer->Header.Length);

        //  Yes, this falls through and finds the RSDT even if an XSDT was found.
    }

find_rsdt:

    RsdtPointer = (acpi_table_rsdt *)(uintptr_t)(((acpi_rsdp_common *)rsdp.GetInvariantValue())->RsdtPhysicalAddress);

    if (RsdtPointer == nullptr)
        return HandleResult::NotFound;

    assert_or(::memeq(RsdtPointer->Header.Signature, ACPI_SIG_RSDT, ACPI_NAME_SIZE)
        , "RSDT signature doesn't appear to be valid..? (\"%S\")%n"
        , ACPI_NAME_SIZE, RsdtPointer->Header.Signature)
    {
        if (XsdtPointer == nullptr)
            return HandleResult::IntegrityFailure;
        else
            return HandleResult::Okay;

        //  Signature invalid? Try to return with a valid XSDT at least.
    }

    uint8_t const sumRsdt = Checksum8(RsdtPointer, RsdtPointer->Header.Length);

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

    ConsiderAddress(RsdtPointer);
    ConsiderAddress((uintptr_t)RsdtPointer + RsdtPointer->Header.Length);
    
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

                //  Yes, let the user know if possible and move on.
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

Handle Acpi::HandleSystemDescriptorTable(paddr_t const paddr, SystemDescriptorTableSource const src)
{
    auto headerPtr = (acpi_table_header *)paddr.Value;

    uint8_t sumXsdt = Checksum8(headerPtr, headerPtr->Length);

    assert_or(0 == sumXsdt
        , "System descriptor table checksum failed: %u1%n"
        , sumXsdt)
    {
        headerPtr = nullptr;

        return HandleResult::IntegrityFailure;
    }

    /*msg("~[ FOUND TABLE \"%S\" @ %Xp (%XP) in %s ]~%n"
        , ACPI_NAME_SIZE, headerPtr->Signature, vaddr, paddr
        , (src == SystemDescriptorTableSource::Xsdt) ? ACPI_SIG_XSDT : ACPI_SIG_RSDT);//*/

    ConsiderAddress(paddr);
    ConsiderAddress(paddr + psize_t(headerPtr->Length));

    if (::memeq(headerPtr->Signature, ACPI_SIG_MADT, ACPI_NAME_SIZE))
        return Acpi::HandleMadt(paddr, src);
    else if (::memeq(headerPtr->Signature, ACPI_SIG_SRAT, ACPI_NAME_SIZE))
        return Acpi::HandleSrat(paddr, src);
    // else
    //     MSG("$ Found unknown ACPI table: %S%n", ACPI_NAME_SIZE, headerPtr->Signature);

    return HandleResult::Okay;
}

Handle Acpi::HandleMadt(paddr_t const paddr, SystemDescriptorTableSource const src)
{
    if (MadtPaddr == paddr || (MadtSrc != src && MadtSrc != SystemDescriptorTableSource::None))
        return HandleResult::Okay;
    //  Same physical address or different source table? No problemo, then.

    assert_or(MadtPointer == nullptr
        , "Duplicate (different) MADTs found under the same table (%s)?!%n"
          "First @ %Xp (%XP);%n"
          "Second @ %XP."
        , (src == SystemDescriptorTableSource::Xsdt) ? ACPI_SIG_XSDT : ACPI_SIG_RSDT
        , MadtPointer, MadtPaddr, paddr)
    {
        return HandleResult::CardinalityViolation;
    }

    MadtPointer = (acpi_table_madt *)(uintptr_t)paddr;
    MadtPaddr = paddr;
    MadtSrc = src;

    uintptr_t madtEnd = paddr.Value + Acpi::MadtPointer->Header.Length;
    uintptr_t e = paddr.Value + sizeof(*Acpi::MadtPointer);
    for (/* nothing */; e < madtEnd; e += ((acpi_subtable_header *)e)->Length)
    {
        switch (((acpi_subtable_header *)e)->Type)
        {
        case ACPI_MADT_TYPE_LOCAL_APIC:
#if   defined(__BEELZEBUB_SETTINGS_SMP)
            {
                auto lapic = (acpi_madt_local_apic *)e;

                if (0 != (ACPI_MADT_ENABLED & lapic->LapicFlags))
                    ++PresentLapicCount;

                ++LapicCount;
            }
#endif
            break;

        case ACPI_MADT_TYPE_IO_APIC:
            ++IoapicCount;
            break;

        case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
            break;

        case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
            break;

        default:
            MSG("%n%*(( MADTe: T%u1 L%u1 ))"
                , 50
                , ((acpi_subtable_header *)e)->Type
                , ((acpi_subtable_header *)e)->Length);
            break;
        }
    }

    return HandleResult::Okay;
}

Handle Acpi::HandleSrat(paddr_t const paddr, SystemDescriptorTableSource const src)
{
    if (SratPaddr == paddr || (SratSrc != src && SratSrc != SystemDescriptorTableSource::None))
        return HandleResult::Okay;
    //  Same physical address or different source table? No problemo, then.

    assert_or(SratPointer == nullptr
        , "Duplicate (different) SRATs found under the same table (%s)?!%n"
          "First @ %Xp (%XP);%n"
          "Second @ %XP."
        , (src == SystemDescriptorTableSource::Xsdt) ? ACPI_SIG_XSDT : ACPI_SIG_RSDT
        , SratPointer, SratPaddr, paddr)
    {
        return HandleResult::CardinalityViolation;
    }

    SratPointer = (acpi_table_srat *)(uintptr_t)paddr;
    SratPaddr = paddr;
    SratSrc = src;

    return HandleResult::Okay;
}

/*  Utilities  */

Handle Acpi::FindLapicPaddr(paddr_t & paddr)
{
    Handle res;

    if (MadtPointer == nullptr)
        return HandleResult::UnsupportedOperation;

    paddr = (paddr_t)MadtPointer->Address;

    //uintptr_t MadtEntriesStart = (uintptr_t)MadtPointer + sizeof(acpi_table_madt);

    return HandleResult::Okay;
}
