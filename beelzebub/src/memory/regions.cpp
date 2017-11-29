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

#include "memory/regions.hpp"

#include <beel/utils/avl.tree.hpp>
#include <beel/terminals/base.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Utils;

typedef AvlTree<MemoryRegion> TreeType;
typedef TreeType::Node NodeType;

/*************************
    MemoryRange struct
*************************/

/*  Statics  */

MemoryRange const MemoryRange::Invalid { vaddr_t(1337), vaddr_t(42) };

/*  Operators  */

MemoryRange MemoryRange::operator & (MemoryRange const & other) const
{
    if (this->End <= other.Start || other.End <= this->Start)
        return Invalid;

    return { vaddr_t(Maximum(this->Start, other.Start)), vaddr_t(Minimum(this->End, other.End)) };
}

MemoryRange MemoryRange::operator | (MemoryRange const & other) const
{
    if (this->End < other.Start || other.End < this->Start)
        return Invalid;

    return { vaddr_t(Minimum(this->Start, other.Start)), vaddr_t(Maximum(this->End, other.End)) };
}

/******************
    Comparables
******************/

namespace Beelzebub { namespace Utils
{
    #define COMP_MRNG_MRNG_IMPL(a, b) \
        if (a.End <= b.Start) \
            return -1; \
        else if (a.Start >= b.End) \
            return 1; \
        else \
            return 0;

    #define COMP_MRNG_VADR_IMPL(a, b) \
        if (a.End <= b) \
            return -1; \
        else if (a.Start > b) \
            return 1; \
        else \
            return 0;

    #define COMP_MREG_AMRE_IMPL(a, b) \
        if (a.Range.End < b.Payload.Range.Start) \
            return -1; \
        else if (a.Range.Start > b.Payload.Range.End) \
            return 1; \
        else \
            return 0;

    COMP_IMPL(MemoryRange ,          MemoryRange, COMP_MRNG_MRNG_IMPL)
    COMP_IMPL(MemoryRange ,              vaddr_t, COMP_MRNG_VADR_IMPL)
    COMP_IMPL(MemoryRegion, AdjacentMemoryRegion, COMP_MREG_AMRE_IMPL)

    COMP_IMPL_REVERSE(vaddr_t, MemoryRange)

    //  The ones above assume the ranges are valid!

    #define GET_REGION_RANGE(reg) reg.Range

    COMP_FORWARD_SINGLE(MemoryRegion, MemoryRange, GET_REGION_RANGE)

    COMP_FORWARD_TWO_WAY(MemoryRegion, MemoryRange, MemoryRange, MemoryRange, GET_REGION_RANGE, MCATS1)
    COMP_FORWARD_TWO_WAY(MemoryRegion, vaddr_t, MemoryRange, vaddr_t, GET_REGION_RANGE, MCATS1)
}}

/************************
    TERMINAL PRINTING
************************/

namespace Beelzebub { namespace Terminals
{
    template<>
    TerminalBase & operator << <MemoryRegion const *>(TerminalBase & term, MemoryRegion const * const reg)
    {
        term.WriteFormat("%X8 %X8 %X8 %c%c%c%c %c%c%c %s%n"
            //  Start         , End           , Size
            , reg->Range.Start, reg->Range.End, reg->GetSize()
            //  Writable, Executable, Global, Userland (flgs)
            , 0 != (reg->Flags & MemoryFlags::Writable  ) ? 'W' : ' '
            , 0 != (reg->Flags & MemoryFlags::Executable) ? 'X' : ' '
            , 0 != (reg->Flags & MemoryFlags::Global    ) ? 'G' : ' '
            , 0 != (reg->Flags & MemoryFlags::Userland  ) ? 'U' : ' '
            //  Permanent, High guard, Low guard (prt)
            , 0 != (reg->Type & MemoryAllocationOptions::Permanent) ? 'P' : ' '
            , 0 != (reg->Type & MemoryAllocationOptions::GuardHigh) ? 'H' : ' '
            , 0 != (reg->Type & MemoryAllocationOptions::GuardLow ) ? 'L' : ' '
            //  Content
            , EnumToString(reg->Content));

        return term;
    }

    template<>
    TerminalBase & operator << <MemoryRegion *>(TerminalBase & term, MemoryRegion * const reg)
    {
        return term << const_cast<MemoryRegion const *>(reg);
    }

    template<>
    TerminalBase & operator << <MemoryRegion>(TerminalBase & term, MemoryRegion const value)
    {
        return term << &value;
    }
}}
