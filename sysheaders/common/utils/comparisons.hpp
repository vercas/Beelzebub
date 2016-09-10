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

namespace Beelzebub { namespace Utils
{
    template<typename TThis, typename TOther>
    __public comp_t Compare(TThis const & a, TOther const & b);
    template<typename TThis, typename TOther>
    __public comp_t Compare(TThis const & a, TOther const && b);
    template<typename TThis, typename TOther>
    __public comp_t Compare(TThis const && a, TOther const & b);
    template<typename TThis, typename TOther>
    __public comp_t Compare(TThis const && a, TOther const && b);

    #define COMP_IMPL_3(TThis, TOther, impl) \
        template<> \
        comp_t Compare<TThis, TOther>(TThis const & a, TOther const & b) \
        { impl(a, b) } \
        template<> \
        comp_t Compare<TThis, TOther>(TThis const & a, TOther const && b) \
        { impl(a, b) } \
        template<> \
        comp_t Compare<TThis, TOther>(TThis const && a, TOther const & b) \
        { impl(a, b) } \
        template<> \
        comp_t Compare<TThis, TOther>(TThis const && a, TOther const && b) \
        { impl(a, b) }

    #define COMP_IMPL_2(TThis, impl) COMP_IMPL_3(TThis, TThis, impl)

    #define COMP_IMPL(arg1, ...) GET_MACRO2(__VA_ARGS__, COMP_IMPL_3, COMP_IMPL_2)(arg1, __VA_ARGS__)

    #define COMP_FORWARD_ONE_WAY(TThis, TOther, TUnderThis, TUnderOther, propThis, propOther) \
        template<> \
        comp_t Compare<TThis, TOther>(TThis const & a, TOther const & b) \
        { \
            return Compare<TUnderThis, TUnderOther>((propThis(a)), (propOther(b))); \
        } \
        template<> \
        comp_t Compare<TThis, TOther>(TThis const & a, TOther const && b) \
        { \
            return Compare<TUnderThis, TUnderOther>((propThis(a)), (propOther(b))); \
        } \
        template<> \
        comp_t Compare<TThis, TOther>(TThis const && a, TOther const & b) \
        { \
            return Compare<TUnderThis, TUnderOther>((propThis(a)), (propOther(b))); \
        } \
        template<> \
        comp_t Compare<TThis, TOther>(TThis const && a, TOther const && b) \
        { \
            return Compare<TUnderThis, TUnderOther>((propThis(a)), (propOther(b))); \
        }

    #define COMP_FORWARD_TWO_WAY(TThis, TOther, TUnderThis, TUnderOther, propThis, propOther) \
        COMP_FORWARD_ONE_WAY(TThis, TOther, TUnderThis, TUnderOther, propThis, propOther) \
        COMP_FORWARD_ONE_WAY(TOther, TThis, TUnderOther, TUnderThis, propOther, propThis)

    #define COMP_FORWARD_SINGLE(TThis, TUnderThis, propThis) \
        COMP_FORWARD_ONE_WAY(TThis, TThis, TUnderThis, TUnderThis, propThis, propThis)
}}
