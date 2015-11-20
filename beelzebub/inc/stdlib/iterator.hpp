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

#include <metaprogramming.h>

//  This implementation is basically my understanding of how this should work.
//  I may be completely wrong about it. I hope I'm not that stupid.
//  The information is obtained from http://en.cppreference.com/w/cpp/iterator/iterator

//  Also, I use a different naming convention!

namespace Std
{
    struct InputIteratorTag {};
    struct OutputIteratorTag {};
    struct ForwardIteratorTag : public InputIteratorTag {};
    struct BidirectionalIteratorTag : public ForwardIteratorTag {};
    struct RandomAccessIteratorTag : public BidirectionalIteratorTag {};

    template<typename category
           , typename type
           , typename distance = ptrdiff_t
           , typename pointer = type *
           , typename reference = type &>
    struct Iterator
    {
        typedef category  IteratorCategory;
        typedef type      ValueType;
        typedef distance  DifferenceType;
        typedef pointer   Pointer;
        typedef reference Reference;
    };

    template<typename iterator>
    struct IteratorTraits
    {
        typedef typename iterator::IteratorCategory  IteratorCategory;
        typedef typename iterator::ValueType         ValueType;
        typedef typename iterator::DifferenceType    DifferenceType;
        typedef typename iterator::Pointer           Pointer;
        typedef typename iterator::Reference         Reference;
    };

    template<typename type>
    struct IteratorTraits<type *>
    {
        typedef RandomAccessIteratorTag     IteratorCategory;
        typedef type                        ValueType;
        typedef ptrdiff_t                   DifferenceType;
        typedef type *                      Pointer;
        typedef type &                      Reference;
    };

    template<typename type>
    struct IteratorTraits<const type *>
    {
        typedef RandomAccessIteratorTag     IteratorCategory;
        typedef type                        ValueType;
        typedef ptrdiff_t                   DifferenceType;
        typedef const type *                Pointer;
        typedef const type &                Reference;
    };
}

