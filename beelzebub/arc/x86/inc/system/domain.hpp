#pragma once

#include <system/gdt.hpp>
#include <memory/page_allocator.hpp>

namespace Beelzebub { namespace System
{
    class Domain
    {
    public:

        /*  Constructor(s)  */

        Domain() = default;

        Domain(Domain const &) = delete;
        Domain & operator =(Domain const &) = delete;

        /*  Field(s)  */

        size_t Index;
        GdtRegister Gdt;
        Memory::PageAllocator * PhysicalAllocator;
    };
}}
