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
 *  The implementation of the virtual memory manager is architecture-specific.
 */

#pragma once

#include <execution/process.hpp>
#include <memory/page_allocator.hpp>
#include <synchronization/atomic.hpp>
#include <handles.h>

namespace Beelzebub { namespace Memory
{
    /**
     *  Represents characteristics of pages that can be mapped.
     */
    enum class MemoryFlags : int
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

    ENUMOPS(MemoryFlags, int)

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
        //  The physical pages will be allocated immediately.
        Commit               = 0x80000000,
        //  No physical pages will be allocated until actually used.
        AllocateOnDemand     = 0xC0000000,

        //  The pages involved will not be freed.
        Permanent            = 0x20000000,
    };

    ENUMOPS(MemoryAllocationOptions, uint32_t)

    /**
     *  The virtual memory manager.
     */
    class Vmm
    {
    public:
        /*  Statics  */

        static Synchronization::Atomic<vaddr_t> KernelHeapCursor;

        /*  Constructor(s)  */

    protected:
        Vmm() = default;

    public:
        Vmm(Vmm const &) = delete;
        Vmm & operator =(Vmm const &) = delete;

        /*  Initialization  */

        __cold static Handle Bootstrap(Execution::Process * const bootstrapProc);
        static Handle Initialize(Execution::Process * const proc);

        /*  Activation and Status  */

        __hot static Handle Switch(Execution::Process * const oldProc
            , Execution::Process * const newProc);

        __hot static bool IsActive(Execution::Process * const proc);
        __hot static bool IsAlien(Execution::Process * const proc);

        /*  Page Management  */

        __hot static __noinline Handle MapPage(Execution::Process * const proc
            , uintptr_t const vaddr, paddr_t const paddr
            , MemoryFlags const flags, PageDescriptor * desc
            , bool const lock = true);

        __hot static __forceinline Handle MapPage(Execution::Process * const proc
            , uintptr_t const vaddr, paddr_t const paddr
            , MemoryFlags const flags, bool const lock = true)
        {
            return MapPage(proc, vaddr, paddr, flags, nullptr, lock);
        }

        __hot static __noinline Handle UnmapPage(Execution::Process * const proc
            , uintptr_t const vaddr, paddr_t & paddr, PageDescriptor * & desc
            , bool const lock = true);

        __hot static __forceinline Handle UnmapPage(Execution::Process * const proc
            , uintptr_t const vaddr, PageDescriptor * & desc, bool const lock = true)
        {
            paddr_t sink;

            return UnmapPage(proc, vaddr, sink, desc, lock);
        }

        __hot static __forceinline Handle UnmapPage(Execution::Process * const proc
            , uintptr_t const vaddr, paddr_t & paddr, bool const lock = true)
        {
            PageDescriptor * sink;

            return UnmapPage(proc, vaddr, paddr, sink, lock);
        }

        __hot static __forceinline Handle UnmapPage(Execution::Process * const proc
            , uintptr_t const vaddr, bool const lock = true)
        {
            paddr_t sink1;
            PageDescriptor * sink2;

            return UnmapPage(proc, vaddr, sink1, sink2, lock);
        }

        __hot static __noinline Handle InvalidatePage(Execution::Process * const proc
            , uintptr_t const vaddr, bool const broadcast = true);

        __hot static __noinline Handle Translate(Execution::Process * const proc
            , uintptr_t const vaddr, paddr_t & paddr);

        /*  Allocation  */

        __hot static __noinline Handle AllocatePages(Execution::Process * const proc
            , size_t const count, MemoryAllocationOptions const type
            , MemoryFlags const flags, uintptr_t & vaddr);

        __hot static __noinline Handle FreePages(Execution::Process * const proc
            , uintptr_t const vaddr, const size_t count);

        /*  Flags  */

        __hot static __noinline Handle GetPageFlags(Execution::Process * const proc
            , uintptr_t const vaddr, MemoryFlags & flags, bool const lock = true);

        __hot static __noinline Handle SetPageFlags(Execution::Process * const proc
            , uintptr_t const vaddr, MemoryFlags const flags, bool const lock = true);
    };
}}
