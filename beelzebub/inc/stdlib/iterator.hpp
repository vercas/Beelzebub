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

