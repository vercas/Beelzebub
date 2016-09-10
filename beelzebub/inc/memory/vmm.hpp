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
#include <memory/enums.hpp>
#include <memory/page_allocator.hpp>
#include <synchronization/atomic.hpp>

namespace Beelzebub { namespace Memory
{
    /**
     *  The virtual memory manager.
     */
    class Vmm
    {
    public:
        /*  Statics  */

        static Synchronization::Atomic<vaddr_t> KernelHeapCursor;

        static vaddr_t UserlandStart;
        static vaddr_t UserlandEnd;
        static vaddr_t KernelStart;
        static vaddr_t KernelEnd;

        /*  Constructor(s)  */

    protected:
        Vmm() = default;

    public:
        Vmm(Vmm const &) = delete;
        Vmm & operator =(Vmm const &) = delete;

        /*  Initialization  */

        __startup static Handle Bootstrap(Execution::Process * const bootstrapProc);
        static Handle Initialize(Execution::Process * proc);

        /*  Activation and Status  */

        __hot static Handle Switch(Execution::Process * const oldProc
            , Execution::Process * const newProc);

        __hot static bool IsActive(Execution::Process * proc);
        __hot static bool IsAlien(Execution::Process * proc);

        /*  Page Management  */

        __hot static __noinline Handle MapPage(Execution::Process * proc
            , uintptr_t const vaddr, paddr_t const paddr
            , MemoryFlags const flags, PageDescriptor * desc
            , bool const lock = true);

        __hot static __forceinline Handle MapPage(Execution::Process * proc
            , uintptr_t const vaddr, paddr_t const paddr
            , MemoryFlags const flags, bool const lock = true)
        {
            return MapPage(proc, vaddr, paddr, flags, nullptr, lock);
        }

        __hot static __noinline Handle UnmapPage(Execution::Process * proc
            , uintptr_t const vaddr, paddr_t & paddr, PageDescriptor * & desc
            , bool const lock = true);

        __hot static __forceinline Handle UnmapPage(Execution::Process * proc
            , uintptr_t const vaddr, PageDescriptor * & desc, bool const lock = true)
        {
            paddr_t sink;

            return UnmapPage(proc, vaddr, sink, desc, lock);
        }

        __hot static __forceinline Handle UnmapPage(Execution::Process * proc
            , uintptr_t const vaddr, paddr_t & paddr, bool const lock = true)
        {
            PageDescriptor * sink;

            return UnmapPage(proc, vaddr, paddr, sink, lock);
        }

        __hot static __forceinline Handle UnmapPage(Execution::Process * proc
            , uintptr_t const vaddr, bool const lock = true)
        {
            paddr_t sink1;
            PageDescriptor * sink2;

            return UnmapPage(proc, vaddr, sink1, sink2, lock);
        }

        __hot static __noinline Handle InvalidatePage(Execution::Process * proc
            , uintptr_t const vaddr, bool const broadcast = true);

        __hot static __noinline Handle Translate(Execution::Process * proc
            , uintptr_t const vaddr, paddr_t & paddr, bool const lock = true);

        __hot static Handle HandlePageFault(Execution::Process * proc
            , uintptr_t const vaddr, PageFaultFlags const flags);

        /*  Allocation  */

        __hot static __noinline Handle AllocatePages(Execution::Process * proc
            , size_t const count, MemoryAllocationOptions const type
            , MemoryFlags const flags, MemoryContent content, uintptr_t & vaddr);

        __hot static __noinline Handle FreePages(Execution::Process * proc
            , uintptr_t const vaddr, size_t const count);

        /*  Flags  */

        __hot static __noinline Handle CheckMemoryRegion(Execution::Process * proc
            , uintptr_t addr, size_t size, MemoryCheckType type);

        __hot static __noinline Handle GetPageFlags(Execution::Process * proc
            , uintptr_t const vaddr, MemoryFlags & flags, bool const lock = true);

        __hot static __noinline Handle SetPageFlags(Execution::Process * proc
            , uintptr_t const vaddr, MemoryFlags const flags, bool const lock = true);
    };
}}
