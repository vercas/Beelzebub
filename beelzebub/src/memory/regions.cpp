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

#include <memory/regions.hpp>
#include <utils/avl_tree.hpp>

#include <memory/object_allocator_smp.hpp>
#include <memory/object_allocator_pools_heap.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Utils;

typedef AvlTree<MemoryRange> TreeType;
typedef TreeType::Node NodeType;

static ObjectAllocatorSmp regionAllocator;

/*************************
    MemoryRange struct
*************************/

/*  Statics  */

MemoryRange const MemoryRange::Invalid {1337, 42};

/*  Operators  */

MemoryRange MemoryRange::operator & (MemoryRange const & other)
{
    if (this->End <= other.Start || other.End <= this->Start)
        return Invalid;

    return {Maximum(this->Start, other.Start), Minimum(this->End, other.End)};
}

MemoryRange MemoryRange::operator | (MemoryRange const & other)
{
    if (this->End < other.Start || other.End < this->Start)
        return Invalid;

    return {Minimum(this->Start, other.Start), Maximum(this->End, other.End)};
}

/******************
    Comparables
******************/

namespace Beelzebub { namespace Utils
{
    template<> template<>
    comp_t Comparable<MemoryRange>::Compare<MemoryRange>(MemoryRange const & other) const
    {
        if (this->Object.End <= other.Start)
            return -1;
        else if (this->Object.Start >= other.End)
            return 1;
        else
            return 0;
    }
    template<> template<>
    comp_t Comparable<MemoryRange>::Compare<MemoryRange>(MemoryRange const && other) const
    {
        if (this->Object.End <= other.Start)
            return -1;
        else if (this->Object.Start >= other.End)
            return 1;
        else
            return 0;
    }

    template<> template<>
    comp_t Comparable<MemoryRegion>::Compare<AdjacentMemoryRegion>(AdjacentMemoryRegion const & other) const
    {
        if (this->Object.Range.End < other.Payload.Range.Start)
            return -1;
        else if (this->Object.Range.Start > other.Payload.Range.End)
            return 1;
        else
        {
            return 0;
        }
    }
    template<> template<>
    comp_t Comparable<MemoryRegion>::Compare<AdjacentMemoryRegion>(AdjacentMemoryRegion const && other) const
    {
        if (this->Object.Range.End < other.Payload.Range.Start)
            return -1;
        else if (this->Object.Range.Start > other.Payload.Range.End)
            return 1;
        else
        {
            return 0;
        }
    }

    //  The ones above assume the ranges are valid!

    template<> template<>
    comp_t Comparable<MemoryRegion>::Compare<MemoryRegion>(MemoryRegion const & other) const
    {
        return (Comparable<MemoryRange>(this->Object.Range)).Compare(other.Range);
    }
    template<> template<>
    comp_t Comparable<MemoryRegion>::Compare<MemoryRegion>(MemoryRegion const && other) const
    {
        return (Comparable<MemoryRange>(this->Object.Range)).Compare(other.Range);
    }

    template<> template<>
    comp_t Comparable<MemoryRegion>::Compare<MemoryRange>(MemoryRange const & other) const
    {
        return (Comparable<MemoryRange>(this->Object.Range)).Compare(other);
    }
    template<> template<>
    comp_t Comparable<MemoryRegion>::Compare<MemoryRange>(MemoryRange const && other) const
    {
        return (Comparable<MemoryRange>(this->Object.Range)).Compare(other);
    }
}}

/*************
    OTHERS
*************/

void Memory::InitializeRegions()
{
    new (&regionAllocator) ObjectAllocatorSmp(sizeof(NodeType), __alignof(NodeType)
        , &AcquirePoolInKernelHeap, &EnlargePoolInKernelHeap, &ReleasePoolFromKernelHeap);
}

namespace Beelzebub { namespace Utils
{
    template<>
    Handle AvlTree<MemoryRange>::AllocateNode(AvlTree<MemoryRange>::Node * & node)
    {
        return regionAllocator.AllocateObject(node);
    }

    template<>
    Handle AvlTree<MemoryRange>::RemoveNode(AvlTree<MemoryRange>::Node * const node)
    {
        return regionAllocator.DeallocateObject(node);
    }
}}
