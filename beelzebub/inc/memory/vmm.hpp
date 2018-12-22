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

#include "execution/process.hpp"
#include "memory/kernel.vas.hpp"
#include "memory/enums.hpp"
#include <beel/enums.kernel.h>

namespace Beelzebub { namespace Memory
{
    /**
     *  The virtual memory manager.
     */
    class Vmm
    {
    public:
        /*  Types  */

        struct RangeInvalidationInfo;
        struct ChainInvalidationInfo;

        typedef void (* AfterRangeInvalidationFunc)(RangeInvalidationInfo const *, bool);
        typedef void (* AfterChainInvalidationFunc)(ChainInvalidationInfo const *, bool);

        struct RangeInvalidationInfo
        {
            Execution::Process * const Proc;
            vaddr_t const * const Addresses;
            size_t const Count, Stride;
            AfterRangeInvalidationFunc const After;
            void * const Cookie;
        };

        struct PageNode
        {
            inline PageNode(vaddr_t addr) : Address( addr), Next(nullptr) { }
            inline PageNode(vaddr_t addr, PageNode const * next) : Address( addr), Next(next) { }

            vaddr_t const Address;
            PageNode const * Next;
        };

        struct ChainInvalidationInfo
        {
            Execution::Process * const Proc;
            PageNode const * const Node;
            AfterChainInvalidationFunc const After;
            void * const Cookie;
        };

        typedef void (* PreUnmapFunc)(Execution::Process * proc
            , vaddr_t vaddr, void * cookie);
        typedef Handle (* PostUnmapFunc)(Execution::Process * proc
            , vaddr_t vaddr, vsize_t size, Handle oRes, void * cookie);

        /*  Statics  */

        static Synchronization::SmpLock KernelHeapLock;

        static KernelVas KVas;

        static vaddr_t UserlandStart;
        static vaddr_t UserlandEnd;
        static vaddr_t KernelStart;
        static vaddr_t KernelEnd;

        /*  Utils  */

        static Handle AcquirePoolForVas(size_t objectSize, size_t headerSize
                                      , size_t minimumObjects
                                      , ObjectPoolBase * & result);

        /*  Constructor(s)  */

    protected:
        Vmm() = default;

    public:
        Vmm(Vmm const &) = delete;
        Vmm & operator =(Vmm const &) = delete;

        /*  Initialization  */

        static __startup Handle Bootstrap(Execution::Process * const bootstrapProc);
        static Handle Initialize(Execution::Process * proc);

        /*  Activation and Status  */

        static __hot Handle Switch(Execution::Process * const oldProc
            , Execution::Process * const newProc);

        static __hot bool IsActive(Execution::Process * proc);
        static __hot bool IsAlien(Execution::Process * proc);

        /*  Page Management  */

        static __hot __solid Handle MapPage(Execution::Process * proc
            , vaddr_t const vaddr, paddr_t paddr
            , FrameSize size
            , MemoryFlags const flags
            , MemoryMapOptions opts = MemoryMapOptions::None);

        static __hot __forceinline Handle MapPage(Execution::Process * proc
            , vaddr_t const vaddr, paddr_t paddr
            , MemoryFlags const flags
            , MemoryMapOptions opts = MemoryMapOptions::None)
        {
            return MapPage(proc, vaddr, paddr, FrameSize::_4KiB, flags, opts);
        }

        static __hot __solid Handle MapRange(Execution::Process * proc
            , vaddr_t vaddr, paddr_t paddr, vsize_t size
            , MemoryFlags const flags
            , MemoryMapOptions opts = MemoryMapOptions::None);

        static __hot __solid Handle UnmapPage(Execution::Process * proc
            , vaddr_t const vaddr, paddr_t & paddr, FrameSize & size
            , MemoryMapOptions opts = MemoryMapOptions::None);

        static __hot __forceinline Handle UnmapPage(Execution::Process * proc
            , vaddr_t const vaddr, paddr_t & paddr, MemoryMapOptions opts = MemoryMapOptions::None)
        {
            FrameSize dummy;

            return UnmapPage(proc, vaddr, paddr, dummy, opts);
        }

        static __hot __forceinline Handle UnmapPage(Execution::Process * proc
            , vaddr_t const vaddr, FrameSize & size, MemoryMapOptions opts = MemoryMapOptions::None)
        {
            paddr_t dummy;

            return UnmapPage(proc, vaddr, dummy, size, opts);
        }

        static __hot __forceinline Handle UnmapPage(Execution::Process * proc
            , vaddr_t const vaddr, MemoryMapOptions opts = MemoryMapOptions::None)
        {
            paddr_t dummy1;
            FrameSize dummy2;

            return UnmapPage(proc, vaddr, dummy1, dummy2, opts);
        }

        static __hot __solid Handle UnmapRange(Execution::Process * proc
            , vaddr_t vaddr, vsize_t size
            , MemoryMapOptions opts = MemoryMapOptions::None
            , PreUnmapFunc pre = nullptr
            , PostUnmapFunc post = nullptr
            , void * cookie = nullptr);

        static __hot __solid Handle InvalidateRange(Execution::Process * proc
            , vaddr_t const * const addresses, size_t count
            , size_t stride = sizeof(void *), bool broadcast = true
            , AfterRangeInvalidationFunc after = nullptr, void * cookie = nullptr);

        static __hot __solid Handle InvalidateChain(Execution::Process * proc
            , PageNode const * node, bool broadcast = true
            , AfterChainInvalidationFunc after = nullptr, void * cookie = nullptr);

        static __hot __forceinline Handle InvalidateChain(Execution::Process * proc
            , PageNode const node, bool broadcast = true
            , AfterChainInvalidationFunc after = nullptr, void * cookie = nullptr)
        {
            return InvalidateChain(proc, &node, broadcast, after, cookie);
        }

        static __hot __forceinline Handle InvalidatePage(Execution::Process * proc
            , vaddr_t const vaddr, bool broadcast = true)
        {
            return InvalidateChain(proc, PageNode(vaddr), broadcast);
        }

        static __hot __solid Handle Translate(Execution::Process * proc
            , vaddr_t const vaddr, paddr_t & paddr, bool const lock = true);

        static __hot Handle HandlePageFault(Execution::Process * proc
            , vaddr_t const vaddr, PageFaultFlags const flags);

        /*  Allocation  */

        static __hot __solid Handle AllocatePages(Execution::Process * proc
            , vsize_t const size, MemoryAllocationOptions const type
            , MemoryFlags const flags, MemoryContent content, vaddr_t & vaddr);

        static __hot __solid Handle FreePages(Execution::Process * proc
            , vaddr_t const vaddr, vsize_t const size);

        /*  Flags  */

        static __hot __solid Handle CheckMemoryRegion(Execution::Process * proc
            , vaddr_t addr, vsize_t size, MemoryCheckType type);

        static __hot __solid Handle GetPageFlags(Execution::Process * proc
            , vaddr_t const vaddr, MemoryFlags & flags, bool const lock = true);

        static __hot __solid Handle SetPageFlags(Execution::Process * proc
            , vaddr_t const vaddr, MemoryFlags const flags, bool const lock = true);
    };
}}
