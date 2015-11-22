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

#include <memory/manager.hpp>
#include <memory/virtual_allocator.hpp>
#include <synchronization/spinlock.hpp>
#include <synchronization/atomic.hpp>
#include <handles.h>
#include <metaprogramming.h>

using namespace Beelzebub::Synchronization;

namespace Beelzebub { namespace Memory
{
    /**
     *  A memory manager for AMD64.
     */
    class MemoryManagerAmd64 : public MemoryManager
    {
    public:

        /*  Statics  */

        static vaddr_t const IsaDmaStart          = 0xFFFF800000000000ULL;
        static vsize_t const IsaDmaLength         = 1ULL << 24;  //  16 MiB
        static vaddr_t const IsaDmaEnd            = IsaDmaStart + IsaDmaLength;

        static vaddr_t const KernelModulesStart   = IsaDmaEnd;
        static vaddr_t const KernelModulesEnd     = 0xFFFF808000000000ULL;
        static vsize_t const KernelModulesLength  = KernelModulesEnd - KernelModulesStart;

        static vaddr_t const PasDescriptorsStart  = KernelModulesEnd;
        static vaddr_t const PasDescriptorsEnd    = 0xFFFF820000000000ULL;
        static vsize_t const PasDescriptorsLength = PasDescriptorsEnd - PasDescriptorsStart;

        static vaddr_t const HandleTablesStart    = PasDescriptorsEnd;
        static vaddr_t const HandleTablesEnd      = 0xFFFF830000000000ULL;
        static vsize_t const HandleTablesLength   = HandleTablesEnd - HandleTablesStart;

        static vaddr_t const KernelHeapStart      = HandleTablesEnd;
        static vaddr_t const KernelHeapEnd        = 0xFFFFFE0000000000ULL;
        static vsize_t const KernelHeapLength     = KernelHeapEnd - KernelHeapStart;
        static vsize_t const KernelHeapPageCount  = KernelHeapLength >> 12;

        static Atomic<vaddr_t> KernelModulesCursor;
        static Spinlock<> KernelModulesLock;

        static Atomic<vaddr_t> PasDescriptorsCursor;
        static Spinlock<> PasDescriptorsLock;

        static Spinlock<> HandleTablesLock;

        static Atomic<vaddr_t> KernelHeapCursor;
        static size_t volatile KernelHeapLockCount;
        static Spinlock<> KernelHeapMasterLock;

        static Spinlock<> KernelBinariesLock;

        /*  Constructors  */

        inline MemoryManagerAmd64()
            : Vas(nullptr)
            , UserLock()
            , UserHeapCursor(0)
        {

        }

        MemoryManagerAmd64(MemoryManagerAmd64 const &) = delete;
        MemoryManagerAmd64 & operator =(MemoryManagerAmd64 const &) = delete;

        inline explicit MemoryManagerAmd64(VirtualAllocationSpace * const vas)
            : Vas(vas)
            , UserLock()
            , UserHeapCursor(1 << 12)
        {

        }

        /*  Components  */

        VirtualAllocationSpace * const Vas;

        /*  Locks  */

        Spinlock<> UserLock;

        /*  Cursors  */

        volatile vaddr_t UserHeapCursor;
    };
}}
