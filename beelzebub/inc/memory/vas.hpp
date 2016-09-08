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

#pragma once

#include <memory/regions.hpp>
#include <memory/enums.hpp>
#include <utils/avl_tree.hpp>
#include <memory/object_allocator.hpp>

#include <synchronization/rw_spinlock.hpp>
#include <synchronization/atomic.hpp>

namespace Beelzebub { namespace Memory
{
    /**
     *  A virtual address space.
     */
    class Vas
    {
    public:

        /*  Constructors  */

        inline Vas()
            : Lock()
            , Alloc()
            , Tree()
            , FirstFree(nullptr)
            , LastSearched(nullptr)
        {
            this->Tree.Cookie = &(this->Alloc);
        }

        Vas(Vas const &) = delete;
        Vas & operator =(Vas const &) = delete;

        Handle Initialize(vaddr_t start, vaddr_t end
            , AcquirePoolFunc acquirer, EnlargePoolFunc enlarger, ReleasePoolFunc releaser
            , PoolReleaseOptions const releaseOptions = PoolReleaseOptions::ReleaseAll
            , size_t const quota = SIZE_MAX);

        /*  Operations  */

        __hot Handle Allocate(vaddr_t & vaddr, size_t pageCnt
            , MemoryFlags flags, MemoryAllocationOptions type, bool lock = true);
        __hot Handle Modify(vaddr_t vaddr, size_t pageCnt
            , MemoryFlags flags, bool lock = true);

        __hot MemoryRegion * FindRegion(vaddr_t vaddr);

        /*  Fields  */

        Synchronization::RwSpinlock Lock;

        ObjectAllocator Alloc;
        Utils::AvlTree<MemoryRegion> Tree;

        MemoryRegion * FirstFree, * LastSearched;
    };
}}
