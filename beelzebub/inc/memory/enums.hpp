/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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
 *  The implementation of the virtual memory manager is architecture-specific.
 */

#pragma once

#include <metaprogramming.h>

namespace Beelzebub { namespace Memory
{
    /**
     *  Represents characteristics of pages that can be mapped.
     */
    enum class MemoryFlags : uint8_t
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

    ENUMOPS(MemoryFlags, uint8_t)

    /**
     *  Represents types of pages that can be allocated.
     */
    enum class MemoryAllocationOptions : uint32_t
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
        //  contiguous. Implies commitement (immediate allocation) when used on
        //  the kernel heap.
        PhysicallyContiguous = 0x00008000,

        //  Guards the lowest page against underflow/underrun.
        GuardLow             = 0x00004000,
        //  Guard the highest page against overflow/overrun.
        GuardHigh            = 0x00002000,

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

        //  Virtual addresses will be reserved for later manual mapping; no automatic
        //  allocation of physical memory will be performed now or on demand.
        //  Any MemoryFlags specified are ignored.
        Reserve              = 0x00000000,
        //  The pages are to be used by the kernel.
        Used                 = 0x40000000,
        //  The physical pages will be allocated immediately.
        Commit               = 0x80000000,
        //  No physical pages will be allocated until actually used.
        AllocateOnDemand     = 0xC0000000,

        //  The pages involved will not be freed.
        Permanent            = 0x20000000,

        //  Shared with other processes.
        Share                = 0x01000000,
        //  A thread's stack.
        ThreadStack          = 0x02000000,
        //  Part of the runtime.
        Runtime              = 0x03000000,

        //  Nothing, waiting to be used.
        Free                 = 0x0F000000,

        PurposeMask          = 0x0F000000,
        StrategyMask         = 0xF0000000,
        UniquenessMask       = 0x0000F000,
    };

    ENUMOPS(MemoryAllocationOptions, uint32_t)

    /**
     *  Represents the flags related to a page fault.
     */
    enum class PageFaultFlags : uint8_t
    {
        Present     = 0x01,
        Write       = 0x02,
        Userland    = 0x04,
        Reserved    = 0x08,
        Execute     = 0x10,
    };

    ENUMOPS(PageFaultFlags, uint8_t)

    /**
     *  Represents flags to check memory for.
     */
    enum class MemoryCheckType
    {
        Readable = 0x0,
        Writable = 0x1,
        Free     = 0x2,
        Userland = 0x4,
    };

    ENUMOPS(MemoryCheckType)
}}
