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

#include <beel/utils/comparisons.hpp>
#include <beel/handles.h>
#include <math.h>

#include <debug.hpp>

namespace Beelzebub { namespace Utils
{
    enum class TreeTraversalOrder
    {
        PreOrder,
        InOrder,
        PostOrder,
        LevelOrder
    };

    template<typename TPayload>
    class AvlTreeNode
    {
    public:
        /*  Constructors  */

        AvlTreeNode() = default;

        /*  Properties and Manipulation  */

        static int GetHeight(AvlTreeNode<TPayload> const * const dis)
        {
            if unlikely(dis == nullptr)
                return 0;
            else
                return dis->Height;
        }

        static int GetBalance(AvlTreeNode<TPayload> const * const dis)
        {
            if unlikely(dis == nullptr)
                return 0;
            else
                return GetHeight(dis->Right) - GetHeight(dis->Left);
        }

        int ComputeHeight()
        {
            return this->Height = Maximum(GetHeight(this->Left), GetHeight(this->Right)) + 1;
        }

        AvlTreeNode<TPayload> * FindMinimum()
        {
            if unlikely(this->Left == nullptr)
                return this;
            else
                return this->Left->FindMinimum();
        }

        AvlTreeNode<TPayload> * FindMaximum()
        {
            if unlikely(this->Right == nullptr)
                return this;
            else
                return this->Right->FindMaximum();
        }

        /*  Operations  */

        AvlTreeNode<TPayload> * RotateLeft()
        {
            AvlTreeNode<TPayload> * const right = this->Right;

            this->Right = right->Left;
            right->Left = this;
            //  Actual rotation.

            this->ComputeHeight();
            right->ComputeHeight();
            //  Height update.

            return right;
            //  This one should replace the current node in the hierarchy.
        }

        AvlTreeNode<TPayload> * RotateRight()
        {
            AvlTreeNode<TPayload> * const left = this->Left;

            this->Left  = left->Right;
            left->Right = this;
            //  Actual rotation.

            this->ComputeHeight();
            left->ComputeHeight();
            //  Height update.

            return left;
            //  This one should replace the current node in the hierarchy.
        }

        AvlTreeNode<TPayload> * Balance()
        {
            this->ComputeHeight();

            int const balance = GetBalance(this);

            if (balance > 1)
            {
                if (GetBalance(this->Right) < 0)
                    this->Right = this->Right->RotateRight();

                return this->RotateLeft();
            }
            else if (balance < -1)
            {
                if (GetBalance(this->Left) > 0)
                    this->Left = this->Left->RotateLeft();

                return this->RotateRight();
            }

            return this;
        }

        /*  Fields  */

        AvlTreeNode<TPayload> * Left, * Right;
        int Height; //  Don't really care about the type.

        TPayload Payload;
    };

    template<typename TPayload>
    class AvlTree
    {
    public:
        /*  Types  */

        typedef AvlTreeNode<TPayload> Node;

        /*  Statics  */

        static constexpr size_t const NodeSize = sizeof(Node);

        static Handle AllocateNode(Node * & node, void * cookie);
        static Handle RemoveNode(Node * node, void * cookie);

        static Handle Create(Node * & node, void * cookie)
        {
            Handle res = AllocateNode(node, cookie);

            if likely(res.IsOkayResult())
            {
                node->Left = node->Right = nullptr;
                node->Height = 1;   //  Always a leaf.
            }

            return res;
        }

        static Handle Create(Node * & node, TPayload const & payload, void * cookie)
        {
            Handle res = Create(node, cookie);

            if likely(res.IsOkayResult())
                node->Payload = payload;

            return res;
        }

        template<typename TKey>
        static TPayload * Find(TKey const & key, Node * node)
        {
            if unlikely(node == nullptr)
                return nullptr;

            comp_t const compRes = Compare(node->Payload, key);
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

        template<typename TPred>
        static TPayload * FindPred(TPred const pred, Node * node)
        {
            if unlikely(node == nullptr)
                return nullptr;

            comp_t const compRes = pred(node);

            if (compRes == 0)
                return &(node->Payload);
            //  Found.

            if (compRes > 0)
                return FindPred<TPred>(pred, node->Left);
            else
                return FindPred<TPred>(pred, node->Right);
        }

        template<typename TCover>
        static Handle InsertOrFind(TCover & cover, Node * & node, void * cookie)
        {
            //  First step is spawning a new node.

            if unlikely(node == nullptr)
                return Create(node, cover.Payload, cookie);
            //  Unlikely because it's literally done once per insertion.

            comp_t const compRes = Compare(node->Payload, cover);
            //  Note the comparison order.

            if unlikely(compRes == 0)
            {
                //  Enforce uniqueness, otherwise problems occur.
                cover.Finding = node->Payload;

                return HandleResult::CardinalityViolation;
            }

            Handle res;

            if (compRes > 0)
                res = InsertOrFind<TCover>(cover, node->Left, cookie);
            else
                res = InsertOrFind<TCover>(cover, node->Right, cookie);
            //  "Lesser" nodes go left, "greater" nodes go right.

            //  Then... Balance, if needed.

            if likely(res.IsOkayResult())
            {
                Node * const replacement = node->Balance();

                if (node != replacement)
                    node = replacement;
            }

            return res;
        }

        static Handle Insert(TPayload const & payload, Node * & node, Node * & newNode, void * cookie)
        {
            Handle res;

            //  First step is spawning a new node.

            if unlikely(node == nullptr)
            {
                //  Unlikely because it's literally done once per insertion.

                res = Create(node, payload, cookie);

                newNode = node;
                return res;
            }

            comp_t const compRes = Compare(node->Payload, payload);
            //  Note the comparison order.

            if unlikely(compRes == 0)
                return HandleResult::CardinalityViolation;
            //  Enforce uniqueness, otherwise problems occur.

            if (compRes > 0)
                res = Insert(payload, node->Left, newNode, cookie);
            else
                res = Insert(payload, node->Right, newNode, cookie);
            //  "Lesser" nodes go left, "greater" nodes go right.

            //  Then... Balance, if needed.

            if likely(res.IsOkayResult())
            {
                Node * const replacement = node->Balance();

                if (node != replacement)
                    node = replacement;
            }

            return res;
        }

        template<typename TKey>
        static Handle InsertBlank(TKey const & key, Node * & node, TPayload * & payload, void * cookie)
        {
            Handle res;

            //  First step is spawning a new node.

            if unlikely(node == nullptr)
            {
                //  Unlikely because it's literally done once per insertion.
                res = Create(node, cookie);

                payload = &(node->Payload);

                return res;
            }

            comp_t const compRes = Compare(node->Payload, key);
            //  Note the comparison order.

            if unlikely(compRes == 0)
                return HandleResult::CardinalityViolation;
            //  Enforce uniqueness, otherwise problems occur.

            if (compRes > 0)
                res = InsertBlank<TKey>(key, node->Left, payload, cookie);
            else
                res = InsertBlank<TKey>(key, node->Right, payload, cookie);
            //  "Lesser" nodes go left, "greater" nodes go right.

            //  Then... Balance, if needed.

            if likely(res.IsOkayResult())
            {
                Node * const replacement = node->Balance();

                if (node != replacement)
                    node = replacement;
            }

            return res;
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

                Node * const replacement = node->Balance();

                if (node != replacement)
                    node = replacement;
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

            comp_t const compRes = Compare(node->Payload, key);
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
            {
                //  No need to balance the tree when nothing was removed.

                Node * const replacement = node->Balance();

                if (node != replacement)
                    node = replacement;
            }

            return res;
        }

        template<typename TKey, typename TPredicate>
        static bool RemoveIf(TKey const & key, TPredicate pred, Node * & node, Node * & find)
        {
            if unlikely(node == nullptr)
                return false;
            //  Not found.

            comp_t const compRes = Compare(node->Payload, key);
            //  Note the comparison order.

            bool res;

            if (compRes > 0)
                res = RemoveIf<TKey, TPredicate>(key, pred, node->Left, find);
            else if (compRes < 0)
                res = RemoveIf<TKey, TPredicate>(key, pred, node->Right, find);
            else if likely(res = pred(node))
            {
                //  This is the node, but removal will only be done if the
                //  predicate says so.

                find = node;

                if (find->Left == nullptr)
                {
                    if (find->Right == nullptr)
                    {
                        node = nullptr;

                        return res;
                    }

                    node = find->Right;
                }
                else if (find->Right == nullptr)
                    node = find->Left;
                else
                {
                    Node * temp = RemoveMinimum(find->Right);

                    temp->Left = find->Left;
                    temp->Right = find->Right;
                    temp->Height = find->Height;

                    node = temp;
                }
            }

            if likely(res)
            {
                Node * const replacement = node->Balance();

                if (node != replacement)
                    node = replacement;
            }

            return res;
        }

        template<typename TLambda>
        static bool IteratePreOrder(Node * const node, TLambda lambda)
        {
            //  false = stop; True = continue;

            if unlikely(node == nullptr)
                return true;

            if unlikely(!lambda(node))
                return false;

            if unlikely(!IteratePreOrder(node->Left, lambda))
                return false;

            return IteratePreOrder(node->Right, lambda);
            //  Tail recursion, hopefully.
        }

        template<typename TLambda>
        static bool IterateInOrder(Node * const node, TLambda lambda)
        {
            //  false = stop; True = continue;

            if unlikely(node == nullptr)
                return true;

            if unlikely(!IterateInOrder(node->Left, lambda))
                return false;

            if unlikely(!lambda(node))
                return false;

            return IterateInOrder(node->Right, lambda);
        }

        template<typename TLambda>
        static bool IteratePostOrder(Node * const node, TLambda lambda)
        {
            //  false = stop; True = continue;

            if unlikely(node == nullptr)
                return true;

            if unlikely(!IteratePostOrder(node->Left, lambda))
                return false;

            if unlikely(!IteratePostOrder(node->Right, lambda))
                return false;

            return lambda(node);
            //  Could be a tail call, but it sure as heck ain't as good as tail
            //  recursion.
        }

        template<typename TLambda>
        static bool IterateLevelOrder(Node * const node, TLambda lambda
            , size_t level, size_t const target, size_t & count)
        {
            //  false = stop; True = continue;

            if unlikely(node == nullptr)
                return true;

            if (level == target)
            {
                ++count;
                //  This is a hit.

                return lambda(node);
            }

            //  Now level can only be < target.

            ++level;
            //  Up the game.

            if unlikely(!IterateLevelOrder(node->Left, lambda, level, target, count))
                return false;

            return IterateLevelOrder(node->Right, lambda, level, target, count);
            //  Tail recursion, hopefully.
        }

        static Handle Clear(Node * const node, void * cookie)
        {
            if unlikely(node == nullptr)
                return HandleResult::Okay;

            Handle const res1 = Clear(node->Left, cookie);
            Handle const res2 = Clear(node->Right, cookie);
            Handle const res3 = RemoveNode(node, cookie);

            if likely(res1.IsOkayResult())
                if likely(res2.IsOkayResult())
                    return res3;
                else
                    return res2;
            else
                return res1;
        }

        /*  Constructors  */

        AvlTree() = default;

        /*  Operations  */

        template<typename TKey>
        TPayload * Find(TKey const * const key)
        {
            return Find<TKey>(*key, this->Root);
        }

        template<typename TKey>
        TPayload * Find(TKey const key)
        {
            TKey dummy = key;

            return Find<TKey>(dummy, this->Root);
        }

        template<typename TPred>
        TPayload * FindPred(TPred const pred)
        {
            return FindPred<TPred>(pred, this->Root);
        }

        template<typename TCover>
        Handle InsertOrFind(TCover & cover)
        {
            return InsertOrFind<TCover>(cover, this->Root, this->Cookie);
        }

        Handle Insert(TPayload const & payload)
        {
            Node * temp;

            Handle res = Insert(payload, this->Root, temp, this->Cookie);

            if likely(res.IsOkayResult())
                ++this->NodeCount;

            return res;
        }

        Handle Insert(TPayload const && payload)
        {
            TPayload const dummy = payload;
            Node * temp;

            Handle res = Insert(dummy, this->Root, temp, this->Cookie);

            if likely(res.IsOkayResult())
                ++this->NodeCount;

            return res;
        }

        Handle Insert(TPayload const & payload, TPayload * & pl)
        {
            Node * temp;

            Handle res = Insert(payload, this->Root, temp, this->Cookie);

            if likely(res.IsOkayResult())
            {
                ++this->NodeCount;

                pl = &(temp->Payload);
            }
            else
                pl = nullptr;

            return res;
        }

        Handle Insert(TPayload const && payload, TPayload * & pl)
        {
            TPayload const dummy = payload;
            Node * temp;

            Handle res = Insert(dummy, this->Root, temp, this->Cookie);

            if likely(res.IsOkayResult())
            {
                ++this->NodeCount;

                pl = &(temp->Payload);
            }
            else
                pl = nullptr;

            return res;
        }

        template<typename TKey>
        Handle InsertBlank(TKey const key, TPayload * & payload)
        {
            Handle res = InsertBlank(key, this->Root, payload, this->Cookie);

            if likely(res.IsOkayResult())
                ++this->NodeCount;

            return res;
        }

        template<typename TKey>
        Handle Remove(TKey const * const key)
        {
            Node * const node = Remove<TKey>(*key, this->Root);

            if (node == nullptr)
                return HandleResult::NotFound;
            else
            {
                --this->NodeCount;

                return RemoveNode(node, this->Cookie);
            }
        }

        template<typename TKey>
        Handle Remove(TKey const key)
        {
            TKey dummy = key;

            Node * const node = Remove<TKey>(dummy, this->Root);

            if (node == nullptr)
                return HandleResult::NotFound;
            else
            {
                --this->NodeCount;

                return RemoveNode(node, this->Cookie);
            }
        }

        template<typename TKey>
        Handle Remove(TKey const * const key, TPayload & res)
        {
            Node * const node = Remove<TKey>(*key, this->Root);

            if (node == nullptr)
                return HandleResult::NotFound;
            else
            {
                res = node->Payload;
                //  Will effectively perform a copy.

                --this->NodeCount;

                return RemoveNode(node, this->Cookie);
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
                res = node->Payload;

                --this->NodeCount;

                return RemoveNode(node, this->Cookie);
            }
        }

        template<typename TKey, typename TPredicate>
        Handle RemoveIf(TKey const & key, TPredicate pred)
        {
            Node * node;

            if unlikely(!(RemoveIf<TKey, TPredicate>(key, pred, this->Root, node)))
                return HandleResult::NotFound;
            else
            {
                --this->NodeCount;

                return RemoveNode(node, this->Cookie);
            }
        }

        /*  Iteration  */

        template<typename TLambda>
        bool Traverse(TreeTraversalOrder const order, TLambda lambda)
        {
            switch (order)
            {
            case TreeTraversalOrder::PreOrder:
                return IteratePreOrder(this->Root, lambda);
            case TreeTraversalOrder::InOrder:
                return IterateInOrder(this->Root, lambda);
            case TreeTraversalOrder::PostOrder:
                return IteratePostOrder(this->Root, lambda);

            case TreeTraversalOrder::LevelOrder:
                size_t level = 0, count;

                do
                {
                    count = 0;

                    if (!IterateLevelOrder(this->Root, lambda, 0, level++, count))
                        return false;
                } while (count > 0);

                return true;
            }

            return false;
        }

        /*  Clearing  */

        Handle Clear()
        {
            Handle res = Clear(this->Root, this->Cookie);

            this->Root = nullptr;
            this->NodeCount = 0;
            //  These ought to be done even on failure.

            return res;
        }

        /*  Fields  */

        Node * Root;
        size_t NodeCount;

        void * Cookie;
    };
}}
