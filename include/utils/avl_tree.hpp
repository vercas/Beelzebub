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

#include <utils/comparables.hpp>
#include <handles.h>
#include <math.h>

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

        AvlTreeNode * RemoveMinimum()
        {
            if unlikely(this->Left == nullptr)
                return this->Right;
            //  No left node means this is the minimum. The right leaf will
            //  replace this one.

            //  TODO: Report?

            this->Left = this->Left->RemoveMinimum();

            return this->Balance();
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

        static Handle Insert(TPayload & payload, Node * & node)
        {
            //  First step is spawning a new node.

            if unlikely(node == nullptr)
                return Create(node, payload);

            comp_t const compRes = node->Payload.Compare(payload);

            if unlikely(compRes == 0)
                return HandleResult::CardinalityViolation;
            //  Enforce uniqueness, otherwise problems occur.

            Handle res;

            if (compRes < 0)
                res = Insert(payload, node->Left);
            else
                res = Insert(payload, node->Right);

            if unlikely(!res.IsOkayResult())
                return res;

            //  Then... Balance, if needed.

            node = node->Balance();

            return HandleResult::Okay;
        }

    public:

        static constexpr size_t const NodeSize = sizeof(Node);

        /*  Constructors  */

        AvlTree() = default;

        /*  Operations  */

        Handle Insert(TPayload & payload)
        {
            return Insert(payload, this->Root);
        }

        Handle Insert(TPayload const && payload)
        {
            TPayload dummy = payload;

            return Insert(dummy, this->Root);
        }

        /*  Fields  */

        Node * Root;
    };
}}
