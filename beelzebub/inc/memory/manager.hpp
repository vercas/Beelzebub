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

/**
 *  The implementation of the main memory manager is architecture-specific:
 *
 *  AMD64       :   manager_amd64.cpp
 */

#pragma once

#include <memory/page_allocator.hpp>
#include <handles.h>

namespace Beelzebub { namespace Memory
{
    /**
     *  Represents characteristics of pages that can be mapped.
     */
    enum class PageFlags : int
    {
        //  No flags.
        None       = 0x0,

        //  Shared by all processes.
        Global     = 0x01,
        //  Accessible by user code.
        Userland   = 0x02,
        //  Writing to the page is allowed.
        Writable   = 0x04,
        //  Executing code from the page is allowed.
        Executable = 0x08,
    };

    ENUMOPS(PageFlags, int)

    /**
     *  Represents the status of memory pages.
     */
    enum class PageStatus : int
    {
        //  No flags.
        None       = 0x0,

        //  The page maps to a physical address.
        Present    = 0x01,
        //  Accessed since the flag was reset.
        Accessed   = 0x02,
        //  Written to since the flag was reset.
        Written    = 0x04,
    };

    ENUMOPS(PageStatus, int)

    /**
     *  Represents types of pages that can be allocated.
     */
    enum class AllocatedPageType : uint32_t
    {
        //  The physical page selected will be suitable for general use
        //  in the kernel heap and applications. Highest preferred.
        PhysicalGeneral      = 0x00000000,
        //  The physical page selected will have a 64-bit address,
        //  suitable for certain devices.
        Physical64bit        = 0x00000000,
        //  The physical page selected will have a 32-bit address,
        //  suitable for certain devices.
        Physical32bit        = 0x00000010,
        //  The physical page selected will have a 24-bit address,
        //  suitable for certain devices.
        Physical24bit        = 0x00000020,
        //  The physical page selected will have a 16-bit address,
        //  suitable for certain devices.
        Physical16bit        = 0x00000030,

        //  When multiple pages are allocated, they will be physically
        //  contiguous.
        PhysicallyContiguous = 0x00008000,

        //  The virtual page will be located in areas specific to the kernel heap.
        VirtualKernelHeap    = 0x00000000,
        //  The virtual page will be located in userland-specific areas.
        VirtualUser          = 0x00080000,
        //  The virtual page will be located in 32-bit memory locations,
        //  suitable for 32-bit applications and kernel modules.
        Virtual32bit         = 0x00100000,
        //  The virtual page will be located in 16-bit memory locations,
        //  suitable for 16-bit applications and kernel modules.
        Virtual16bit         = 0x00300000,
    };

    ENUMOPS(AllocatedPageType, uint32_t)

    /**
     *  A memory management unit.
     */
    class MemoryManager
    {
    public:

        /*  Statics  */

        __cold static void Initialize();

        /*  Status  */

        __hot Handle Activate();
        __hot Handle Switch(MemoryManager * const other);
        bool IsActive();

        /*  Page Management  */

        __hot Handle MapPage(vaddr_t const vaddr, paddr_t const paddr, PageFlags const flags, PageDescriptor * const desc);
        __hot Handle MapPage(vaddr_t const vaddr, paddr_t const paddr, PageFlags const flags);
        __hot Handle UnmapPage(vaddr_t const vaddr);
        __hot Handle UnmapPage(vaddr_t const vaddr, PageDescriptor * & desc);
        __hot Handle TryTranslate(vaddr_t const vaddr, paddr_t & paddr);

        __hot Handle AllocatePages(const size_t count, AllocatedPageType const type, PageFlags const flags, vaddr_t & vaddr);
        __hot Handle FreePages(vaddr_t const vaddr, const size_t count);

        /*  Flags  */

        Handle GetPageFlags(vaddr_t const vaddr, PageFlags & flags);
        Handle SetPageFlags(vaddr_t const vaddr, PageFlags const flags);
    };
}}
