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

#include <tests/avl_tree.hpp>
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

#define INSERT_NODE(k, v) do {                                      \
    res = Tree.Insert({(k), (v)});                                  \
    ASSERT(res.IsOkayResult(), "Failed to insert node: %H.", res);  \
} while (false)

#define REMOVE_NODE(k, v) do {                                      \
    TestPayload pl;                                                 \
    res = Tree.Remove((k), pl);                                     \
    ASSERT(res.IsOkayResult(), "Failed to insert node: %H.", res);  \
    ASSERT(pl->Value == (v)                                         \
        , "Found payload %Xp with value %i4, expected %i4."         \
        , pl, pl->Value, (k));                                      \
} while (false)

#define CHECK_KEY(n, v)                         \
    ASSERT((n)->Payload.Object.Key == (v)       \
        , "Root's key should be %i4, not %i4."  \
        , (v), (n)->Payload.Object.Key)

#define FIND_NODE(k, v) do {                                            \
    TestPayload * pl = Tree.Find<int>((k));                             \
    ASSERT(pl != nullptr, "Failed to find node with key %i4.", (k));    \
    ASSERT(pl->Value == (v)                                             \
        , "Found payload %Xp with value %i4, expected %i4."             \
        , pl, pl->Value, (k));                                          \
} while (false)

#define FIND_NO_NODE(k) do {                                            \
    TestPayload * pl = Tree.Find<int>((k));                             \
    ASSERT(pl == nullptr                                                \
        , "Should not have found node %Xp with key %i4."                \
        , pl, (k));                                                     \
} while (false)

void TestAvlTree()
{
    Handle res;

    new (&testAllocator) ObjectAllocatorSmp(sizeof(NodeType), __alignof(NodeType)
        , &AcquirePoolInKernelHeap, &EnlargePoolInKernelHeap, &ReleasePoolFromKernelHeap);

    ASSERT(Tree.Root == nullptr
        , "Empty tree's root should be null, not %Xp."
        , Tree.Root);

    INSERT_NODE(0, 1);

    NodeType * n1 = Tree.Root;

    ASSERT(n1 != nullptr, "Tree root should not be null!");

    CHECK_KEY(n1,   0);

    INSERT_NODE(-10, 2);

    NodeType * n2 = Tree.Root->Left;

    ASSERT(n2 != nullptr, "Tree root's left leaf should not be null!");

    CHECK_KEY(n1,   0);
    CHECK_KEY(n2, -10);

    INSERT_NODE(10, 3);

    NodeType * n3 = Tree.Root->Right;

    ASSERT(n3 != nullptr, "Tree root's right leaf should not be null!");

    CHECK_KEY(n1,   0);
    CHECK_KEY(n2, -10);
    CHECK_KEY(n3,  10);
    //  So this level is stable.

    INSERT_NODE(-20, 4);
    NodeType * n4 = Tree.Root->Left->Left;
    ASSERT(n4 != nullptr, "Node should not be null!");
    CHECK_KEY(n4, -20);

    INSERT_NODE(20, 5);
    NodeType * n5 = Tree.Root->Right->Right;
    ASSERT(n5 != nullptr, "Node should not be null!");
    CHECK_KEY(n5, 20);

    INSERT_NODE(-5, 6);
    NodeType * n6 = Tree.Root->Left->Right;
    ASSERT(n6 != nullptr, "Node should not be null!");
    CHECK_KEY(n6, -5);

    //  Still balanced.
    //  Time to change that.

    INSERT_NODE(30, 7);

    CHECK_KEY(Tree.Root->Right, 20);
    CHECK_KEY(Tree.Root->Right->Left, 10);
    CHECK_KEY(Tree.Root->Right->Right, 30);
    //  A left rotation should've been performed.

    INSERT_NODE(-30, 8);
    INSERT_NODE(-35, 9);

    CHECK_KEY(Tree.Root->Left->Left, -30);
    CHECK_KEY(Tree.Root->Left->Left->Left, -35);
    CHECK_KEY(Tree.Root->Left->Left->Right, -20);
    //  A right rotation should've been performed.

    //  Some queries

    FIND_NODE(  0, 1);
    FIND_NODE(-10, 2);
    FIND_NODE( 10, 3);
    FIND_NODE(-20, 4);
    FIND_NODE( 20, 5);
    FIND_NODE( -5, 6);
    FIND_NODE( 30, 7);
    FIND_NODE(-30, 8);
    FIND_NODE(-35, 9);

    FIND_NO_NODE(  5);
    FIND_NO_NODE( 15);
    FIND_NO_NODE(-15);
    FIND_NO_NODE( 25);
    FIND_NO_NODE(-25);
    FIND_NO_NODE( 35);
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
    template<> template<>
    comp_t Comparable<TestPayload>::Compare<TestPayload>(TestPayload const && other) const
    {
        return (Comparable<int>(this->Object.Key)).Compare(other.Key);
    }

    template<> template<>
    comp_t Comparable<TestPayload>::Compare<int>(int const & other) const
    {
        return (Comparable<int>(this->Object.Key)).Compare(other);
    }
    template<> template<>
    comp_t Comparable<TestPayload>::Compare<int>(int const && other) const
    {
        return (Comparable<int>(this->Object.Key)).Compare(other);
    }
}}

#endif
