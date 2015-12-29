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

#include <execution/images.hpp>

#include <execution/elf.hpp>
#include <utils/avl_tree.hpp>

#include <memory/object_allocator_smp.hpp>
#include <memory/object_allocator_pools_heap.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Utils;

/*  Types  */

typedef AvlTree<Image> TreeType;
typedef TreeType::Node NodeType;

/*  Globals  */

ObjectAllocatorSmp Allocator;
TreeType Repository;

/*******************
    Images class
*******************/

/*  Initialization  */

void Images::Initialize()
{
    new (&Allocator) ObjectAllocatorSmp(sizeof(NodeType), __alignof(NodeType)
        , &AcquirePoolInKernelHeap, &EnlargePoolInKernelHeap, &ReleasePoolFromKernelHeap
        , PoolReleaseOptions::KeepOne, 0, Limit);
}

/*  (Un)load  */

Handle Images::Load(char const * const name, ImageRole const role
    , uint8_t * const imgStart, size_t const size, Image * & img
    , ImageUnloadCallback ucbk)
{
    Handle res;

    res = Repository.InsertBlank<char const *>(name, img);

    if unlikely(!res.IsOkayResult())
        return res;

    //  Basic info.

    img->SetReferenceCount(0);

    img->Name = name;

    img->Start = imgStart;
    img->End = imgStart + size;
    img->Size = size;

    img->Type = ImageType::Invalid;
    img->Role = role;   //  `Auto` will be handled in parsing.

    //  Finding the type.

    img->Parse();

    return res;
}

Handle Images::Unload(Image * const img)
{
    Handle res;

    return HandleResult::Okay;
}

/***********************
    Image allocation
***********************/

namespace Beelzebub { namespace Utils
{
    template<>
    Handle AvlTree<Image>::AllocateNode(AvlTree<Image>::Node * & node)
    {
        return Allocator.AllocateObject(node);
    }

    template<>
    Handle AvlTree<Image>::RemoveNode(AvlTree<Image>::Node * const node)
    {
        return Allocator.DeallocateObject(node);
    }

    template<> template<>
    comp_t Comparable<Image>::Compare<Image>(Image const & other) const
    {
        return (Comparable<char const *>(this->Object.Name)).Compare(other.Name);
    }
    template<> template<>
    comp_t Comparable<Image>::Compare<Image>(Image const && other) const
    {
        return (Comparable<char const *>(this->Object.Name)).Compare(other.Name);
    }

    template<> template<>
    comp_t Comparable<Image>::Compare<char const *>(char const * const & other) const
    {
        return (Comparable<char const *>(this->Object.Name)).Compare(other);
    }
    template<> template<>
    comp_t Comparable<Image>::Compare<char const *>(char const * const && other) const
    {
        return (Comparable<char const *>(this->Object.Name)).Compare(other);
    }
}}

/******************
    Image class
******************/

/*  Reference Count  */

Handle Image::DecrementReferenceCount(size_t & newCount)
{
    newCount = (this->ReferenceCount -= 2) >> 1;

    if (newCount == 0)
        return Images::Unload(this);

    return HandleResult::Okay;
}

/*  Operations  */

void Image::Parse()
{
    if (this->Type == ImageType::Invalid)
        return;
}
