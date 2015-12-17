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

#ifdef __BEELZEBUB__TEST_AVL_TREE

#include <tests/exceptions.hpp>
#include <utils/avl_tree.hpp>

#include <memory/object_allocator_smp.hpp>
#include <memory/object_allocator_pools_heap.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Utils;

struct TestPayload
{
    int Key;
    int Value;
};

typedef AvlTree<TestPayload> TreeType;
typedef TreeType::Node NodeType;

static ObjectAllocatorSmp testAllocator;
static TreeType Tree;

#define CHECK_KEY(n, v)                         \
    ASSERT((n)->Payload.Object.Key == (v)       \
        , "Root's key should be %i4, not %i4."  \
        , (v), (n)->Payload.Object.Key)

void TestAvlTree()
{
    Handle res;

    new (&testAllocator) ObjectAllocatorSmp(sizeof(NodeType), __alignof(NodeType)
        , &AcquirePoolInKernelHeap, &EnlargePoolInKernelHeap, &ReleasePoolFromKernelHeap);

    ASSERT(Tree.Root == nullptr
        , "Empty tree's root should be null, not %Xp."
        , Tree.Root);

    res = Tree.Insert({0, 1});
    ASSERT(res.IsOkayResult(), "Failed to insert node: %H.", res);

    NodeType * n1 = Tree.Root;

    ASSERT(n1 != nullptr, "Tree root should not be null!");

    res = Tree.Insert({-1, 2});
    ASSERT(res.IsOkayResult(), "Failed to insert node: %H.", res);

    res = Tree.Insert({1, 3});
    ASSERT(res.IsOkayResult(), "Failed to insert node: %H.", res);

    NodeType * n2 = Tree.Root->Left, * n3 = Tree.Root->Right;

    ASSERT(n2 != nullptr, "Tree root's left leaf should not be null!");
    ASSERT(n3 != nullptr, "Tree root's right leaf should not be null!");

    CHECK_KEY(n1,  0);
    CHECK_KEY(n2, -1);
    CHECK_KEY(n3,  1);
}

namespace Beelzebub { namespace Utils
{
    template<>
    Handle AvlTree<TestPayload>::AllocateNode(AvlTree<TestPayload>::Node * & node)
    {
        return testAllocator.AllocateObject(node);
    }

    template<>
    Handle AvlTree<TestPayload>::RemoveNode(AvlTree<TestPayload>::Node * const node)
    {
        return testAllocator.DeallocateObject(node);
    }

    template<> template<>
    comp_t Comparable<TestPayload>::Compare<TestPayload>(TestPayload const & other) const
    {
        return (Comparable<int>(this->Object.Key)).Compare(other.Key);
    }
}}

#endif
