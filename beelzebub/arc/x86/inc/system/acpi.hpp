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

#pragma once

#include <utils/tagged_pointer.hpp>
#include <handles.h>

namespace Beelzebub { namespace System
{
    typedef  uint8_t UINT8 ;
    typedef uint16_t UINT16;
    typedef uint32_t UINT32;
    typedef uint64_t UINT64;

    typedef paddr_t ACPI_PHYSICAL_ADDRESS;
    //  Meh.

    typedef UINT8                           ACPI_OWNER_ID;
    #define ACPI_OWNER_ID_MAX               0xFF

    #define ACPI_NAME_SIZE                  4
    #define ACPI_OEM_ID_SIZE                6
    #define ACPI_OEM_TABLE_ID_SIZE          8

    #include "actbl.h"
    //  This file ain't a tru' header!

    /**
     *  <summary>Known ACPI versions.</summary>
     */
    enum class AcpiVersion : uint8_t
    {
        v1  = 0,
        v2  = 1,
    };

    /**
     *  <summary>
     *  Known ACPI tables pointing to system descriptor tables.
     *  </summary>
     */
    enum class SystemDescriptorTableSource
    {
        None = 0,
        Rsdt = 1,
        Xsdt = 2,
    };

    TAGPTR_BEGIN(RsdpPtr, Version, 4, AcpiVersion)
        TAGPTR_TYPE(RsdpPtr, AcpiVersion::v1, Version1, acpi_rsdp_common *)
        TAGPTR_TYPE(RsdpPtr, AcpiVersion::v2, Version2, acpi_table_rsdp *)
    TAGPTR_END()

    /**
     *  <summary>Contains methods for interfacing with the ACPI.</summary>
     */
    class Acpi
    {
    public:
        /*  Statics  */

        static RsdpPtr           RsdpPointer;
        static acpi_table_rsdt * RsdtPointer;
        static acpi_table_xsdt * XsdtPointer;
        static acpi_table_madt * MadtPointer;
        static acpi_table_srat * SratPointer;

#if   defined(__BEELZEBUB_SETTINGS_SMP)
        static size_t LapicCount;
        static size_t PresentLapicCount;
#endif
        
        static size_t IoapicCount;

        /*  Constructor(s)  */

    protected:
        Acpi() = default;

    public:
        Acpi(Acpi const &) = delete;
        Acpi & operator =(Acpi const &) = delete;

        /*  Initialization  */

        static __cold __bland Handle FindRsdp(uintptr_t const start
                                            , uintptr_t const end);

        static __cold __bland Handle FindRsdtXsdt();

        static __cold __bland Handle FindSystemDescriptorTables();

    private:
        /*  Specific table handling  */

        static __cold __bland Handle HandleSystemDescriptorTable(paddr_t const paddr, SystemDescriptorTableSource const src);

        static __cold __bland Handle HandleMadt(vaddr_t const vaddr, paddr_t const paddr, SystemDescriptorTableSource const src);
        static __cold __bland Handle HandleSrat(vaddr_t const vaddr, paddr_t const paddr, SystemDescriptorTableSource const src);

        /*  Utilities  */

        static __cold __bland Handle MapTable(paddr_t const header, vaddr_t & ptr);

    public:
        static __cold __bland Handle FindLapicPaddr(paddr_t & paddr);
    };
}}
