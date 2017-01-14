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

#include "memory/vas.hpp"
#include <beel/sync/atomic.hpp>

namespace Beelzebub { namespace Memory
{
    /**
     *  The kernel's virtual address space.
     */
    class KernelVas : public Vas
    {
        /*  Statics  */

        static constexpr size_t const SpecialNoCore = ~(0UL);

        static constexpr size_t const FreeDescriptorsThreshold = 4;

    public:
        /*  Constructors  */

        KernelVas();

        KernelVas(KernelVas const &) = delete;
        KernelVas & operator =(KernelVas const &) = delete;

        /*  Support  */

        // virtual __hot Handle AllocateNode(Utils::AvlTree<MemoryRegion>::Node * & node) override;
        // virtual __hot Handle RemoveNode(Utils::AvlTree<MemoryRegion>::Node * const node) override;

        virtual __hot Handle PreOp(bool & lock, bool alloc) override;
        virtual __hot Handle PostOp(Handle res, bool lock, bool alloc) override;

        /*  Fields  */

        size_t EnlargingCore;
        bool Bootstrapping;

        // Utils::AvlTree<MemoryRegion>::Node * PreallocatedDescriptors[FreeDescriptorsThreshold];
    };
}}
