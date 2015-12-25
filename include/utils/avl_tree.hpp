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

/*  I just need to take the time to say, that I looked at 12 (!) different AVL
    tree implementations and they all had issues. Some used the wrong order/sign
    for comparisons and balance factor, others used operations which were much
    more complex (asymptotically, in time) than needed... Some lacked removal,
    and all those which had removal implemented had severe issues: incomplete
    code (literally garbage characters all over the code!), logic errors
    (deleting a node just to reallocate it immediately, wrong interpretation of
    comparisons/balance) and/or leaked memory...

    If you need something done well, do it yourself.
*/

#pragma once

#include <utils/comparables.hpp>
#include <handles.h>
#include <math.h>

#include <debug.hpp>

namespace Beelzebub { namespace Utils
{
    template<typename TPayload>
    class AvlTreeNode
    {
    public:
        /*  Constructors  */

        AvlTreeNode() = default;

        /*  Properties and Manipulation  */

        int GetHeight() const
        {
            //  Yes, `this` can be null.

            if unlikely(this == nullptr)
                return 0;
            else
                return this->Height;
        }

        int GetBalance() const
        {
            if unlikely(this == nullptr)
                return 0;
            else
                return this->Right->GetHeight() - this->Left->GetHeight();
        }

        int ComputeHeight()
        {
            return this->Height = Maximum(this->Left->GetHeight(), this->Right->GetHeight()) + 1;
        }

        AvlTreeNode * FindMinimum()
        {
            if unlikely(this->Left == nullptr)
                return this;
            else
                return this->Left->FindMinimum();
        }

        AvlTreeNode * FindMaximum()
        {
            if unlikely(this->Right == nullptr)
                return this;
            else
                return this->Right->FindMaximum();
        }

        /*  Operations  */

        AvlTreeNode * RotateLeft()
        {
            AvlTreeNode * const right = this->Right;

            this->Right = right->Left;
            right->Left = this;
            //  Actual rotation.

            this->ComputeHeight();
            right->ComputeHeight();
            //  Height update.

            return right;
            //  This one should replace the current node in the hierarchy.
        }

        AvlTreeNode * RotateRight()
        {
            AvlTreeNode * const left = this->Left;

            this->Left  = left->Right;
            left->Right = this;
            //  Actual rotation.

            this->ComputeHeight();
            left->ComputeHeight();
            //  Height update.

            return left;
            //  This one should replace the current node in the hierarchy.
        }

        AvlTreeNode * Balance()
        {
            this->ComputeHeight();

            int const balance = this->GetBalance();

            if (balance > 1)
            {
                if (this->Right->GetBalance() < 0)
                    this->Right = this->Right->RotateRight();

                return this->RotateLeft();
            }
            else if (balance < -1)
            {
                if (this->Left->GetBalance() > 0)
                    this->Left = this->Left->RotateLeft();

                return this->RotateRight();
            }

            return this;
        }

        /*  Fields  */

        AvlTreeNode * Left, * Right;
        int Height; //  Don't really care about the type.

        Comparable<TPayload> Payload;
    };

    template<typename TPayload>
    class AvlTree
    {
    public:
        /*  Types  */

        typedef AvlTreeNode<TPayload> Node;

    private:
        /*  Statics  */

        static Handle AllocateNode(Node * & node);
        static Handle RemoveNode(Node * const node);

        static Handle Create(Node * & node, TPayload & payload)
        {
            Handle res = AllocateNode(node);

            if likely(res.IsOkayResult())
            {
                node->Left = node->Right = nullptr;
                node->Height = 1;   //  Always a leaf.

                node->Payload = payload;
            }

            return res;
        }

        template<typename TKey>
        static Comparable<TPayload> * Find(TKey const & key, Node * & node)
        {
            if unlikely(node == nullptr)
                return nullptr;

            comp_t const compRes = node->Payload.Compare(key);
            //  Note the comparison order.

            if (compRes == 0)
                return &(node->Payload);
            //  Found.

            if (compRes > 0)
                return Find<TKey>(key, node->Left);
            else
                return Find<TKey>(key, node->Right);
            //  "Lesser" nodes go left, "greater" nodes go right.
            //  These better be tail calls.
        }

        static Handle Insert(TPayload & payload, Node * & node)
        {
            //  First step is spawning a new node.

            if unlikely(node == nullptr)
                return Create(node, payload);
            //  Unlikely because it's literally done once per insertion.

            comp_t const compRes = node->Payload.Compare(payload);
            //  Note the comparison order.

            if unlikely(compRes == 0)
                return HandleResult::CardinalityViolation;
            //  Enforce uniqueness, otherwise problems occur.

            Handle res;

            if (compRes > 0)
                res = Insert(payload, node->Left);
            else
                res = Insert(payload, node->Right);
            //  "Lesser" nodes go left, "greater" nodes go right.

            if unlikely(!res.IsOkayResult())
                return res;

            //  Then... Balance, if needed.

            node = node->Balance();

            return HandleResult::Okay;
        }

        static Node * RemoveMinimum(Node * & node)
        {
            if unlikely(node == nullptr)
                return nullptr;
            //  There is no minimum in this case.

            Node * res;

            if unlikely(node->Left == nullptr)
            {
                res = node;
                //  This is the minimum.

                node = node->Right;
                //  The node is replaced by its right child. Even when null!

                //  No balancing needed. No height recalculation needed either.
                //  The right child will have the correct height.
            }
            else
            {
                res = RemoveMinimum(node->Left);
                //  The left node's smaller than this one, so the minimum may
                //  be its child!

                node = node->Balance();
                //  Are we AVL trees or are we not?
            }

            return res;
        }

        template <typename TKey>
        static Node * Remove(TKey const & key, Node * & node)
        {
            if unlikely(node == nullptr)
                return nullptr;
            //  Not found.

            comp_t const compRes = node->Payload.Compare(key);
            //  Note the comparison order.

            Node * res;

            if (compRes > 0)
                res = Remove<TKey>(key, node->Left);
            else if (compRes < 0)
                res = Remove<TKey>(key, node->Right);
            else    //  "Lesser" nodes go left, "greater" nodes go right.
            {
                //  This is the node!

                res = node;
                //  Now `res` can be used to retrieve values from `node` a bit
                //  quicker. (doesn't need dereferencing)

                if (res->Left == nullptr)
                {
                    //  This will be the case for the minimum node removed
                    //  below.

                    if (res->Right == nullptr)
                    {
                        //  No children.

                        node = nullptr;

                        return res;
                        //  No need to perform a balance here, so it returns
                        //  early.
                    }

                    //  No left child, but a right child.

                    node = res->Right;
                    //  It is replaced by its right child.
                }
                else if (res->Right == nullptr)
                {
                    //  A left child and no right child.

                    node = res->Left;
                    //  Now it is replaced by its left child.
                }
                else
                {
                    //  Both left and right children are present.

                    Node * temp = RemoveMinimum(res->Right);
                    //  Obtains minimum that is larger than the current.
                    //  Aka the successor. It's also removed from the tree
                    //  in the process.

                    //  At this point, `temp` is dangling. No reference to it
                    //  exists anymore, so it's available for substitution.

                    temp->Left = res->Left;
                    temp->Right = res->Right;
                    temp->Height = res->Height;
                    //  Compiler could optimize the first or last two of these
                    //  to use SSE, I suppose.

                    node = temp;
                    //  Now the successor replaces the current node.
                }
            }

            if likely(res != nullptr)
                node = node->Balance();
            //  No need to balance the tree when nothing was removed.

            return res;
        }

    public:

        static constexpr size_t const NodeSize = sizeof(Node);

        /*  Constructors  */

        AvlTree() = default;

        /*  Operations  */

        template<typename TKey>
        Comparable<TPayload> * Find(TKey const * const key)
        {
            return Find<TKey>(*key, this->Root);
        }

        template<typename TKey>
        Comparable<TPayload> * Find(TKey const key)
        {
            TKey dummy = key;

            return Find<TKey>(dummy, this->Root);
        }

        Handle Insert(TPayload & payload)
        {
            return Insert(payload, this->Root);
        }

        Handle Insert(TPayload const && payload)
        {
            TPayload dummy = payload;

            return Insert(dummy, this->Root);
        }

        template<typename TKey>
        Handle Remove(TKey const * const key)
        {
            Node * const node = Remove<TKey>(*key, this->Root);

            if (node == nullptr)
                return HandleResult::NotFound;
            else
                return RemoveNode(node);
        }

        template<typename TKey>
        Handle Remove(TKey const key)
        {
            TKey dummy = key;

            Node * const node = Remove<TKey>(dummy, this->Root);

            if (node == nullptr)
                return HandleResult::NotFound;
            else
                return RemoveNode(node);
        }

        template<typename TKey>
        Handle Remove(TKey const * const key, TPayload & res)
        {
            Node * const node = Remove<TKey>(*key, this->Root);

            if (node == nullptr)
                return HandleResult::NotFound;
            else
            {
                res = node->Payload.Object;
                //  Will effectively perform a copy.

                return RemoveNode(node);
            }
        }

        template<typename TKey>
        Handle Remove(TKey const key, TPayload & res)
        {
            TKey dummy = key;

            Node * const node = Remove<TKey>(dummy, this->Root);

            if (node == nullptr)
                return HandleResult::NotFound;
            else
            {
                res = node->Payload.Object;

                return RemoveNode(node);
            }
        }

        /*  Fields  */

        Node * Root;
    };
}}
