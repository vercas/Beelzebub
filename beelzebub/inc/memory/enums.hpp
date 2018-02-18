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

#include <beel/metaprogramming.h>

namespace Beelzebub { namespace Memory
{
    /**
     *  Represents types of pages that can be allocated.
     */
    enum class MemoryAllocationOptions : uint32_t
    {
        //  Nothing special
        None                 = 0x00000000,

        //  The pages involved will not be freed.
        Permanent            = 0x00000001,

        //  Guards the lowest page against underflow/underrun.
        GuardLow             = 0x00000004,
        //  Guard the highest page against overflow/overrun.
        GuardHigh            = 0x00000002,

        //  Both lowest and highest pages are guarded against under- and overflows/overruns.
        GuardFull            = 0x00000006,

        //  The virtual page will be located in areas specific to the kernel heap.
        VirtualKernelHeap    = 0x00000000,
        //  The virtual page will be located in userland-specific areas.
        VirtualUser          = 0x80000000,

        //  Virtual addresses will be reserved for later manual mapping; no automatic
        //  allocation of physical memory will be performed now or on demand.
        //  Any MemoryFlags specified are ignored.
        Reserve              = 0x00000000,
        //  The pages are to be managed internally by the kernel.
        Used                 = 0x00000040,
        //  The physical pages will be allocated immediately.
        Commit               = 0x00000080,
        //  No physical pages will be allocated until actually used.
        AllocateOnDemand     = 0x000000C0,

        StrategyMask         = 0x000000F0,
        UniquenessMask       = 0x0000000F,
    };

    __ENUMOPS(MemoryAllocationOptions, uint32_t)

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

    __ENUMOPS(PageFaultFlags, uint8_t)
}}
