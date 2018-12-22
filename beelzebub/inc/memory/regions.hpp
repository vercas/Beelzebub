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

#include <memory/enums.hpp>
#include <beel/enums.kernel.h>

namespace Beelzebub { namespace Memory
{
    struct MemoryRange
    {
        /*  Statics  */

        static MemoryRange const Invalid;

        /*  Constructors  */
        
        inline constexpr MemoryRange()
            : Start(1337)
            , End(42)
        {

        }
        
        inline constexpr MemoryRange(vaddr_t const start, vaddr_t const end)
            : Start(start)
            , End(end)
        {

        }

        /*  Properties  */

        inline bool IsValid() const
        {
            return this->Start.Value != 1337 || this->End.Value != 42;
        }

        inline vsize_t GetSize() const
        {
            return this->End - this->Start;
        }

        inline size_t GetPageCount() const
        {
            return this->GetSize().Value / PageSize.Value;
        }

        /*  Operators  */

        MemoryRange operator & (MemoryRange const & other) const; //  Intersection
        MemoryRange operator | (MemoryRange const & other) const; //  Union

        inline bool operator == (MemoryRange const & other) const
        {
            return this->Start == other.Start && this->End == other.End;
        }

        /*  Queries  */

        inline bool IsIn(MemoryRange const & other) const
        {
            return this->operator == (this->operator & (other));
            //  aka *this == (*this & other) but fancier.
        }

        inline bool Contains(vaddr_t vaddr) const
        {
            return vaddr >= this->Start && vaddr < this->End;
        }

        /*  Fields  */

        vaddr_t Start, End;
    };

    struct MemoryRegion
    {
        /*  Constructors  */

        inline constexpr MemoryRegion()
            : Range()
            , Flags()
            , Type()
            , Content()
            , Next(nullptr)
            , Prev(nullptr)
        {

        }

        inline constexpr MemoryRegion(MemoryRange range
                                    , MemoryFlags flags
                                    , MemoryContent content
                                    , MemoryAllocationOptions type)
            : Range( range)
            , Flags(flags)
            , Type(type)
            , Content(content)
            , Next(nullptr)
            , Prev(nullptr)
        {

        }

        inline constexpr MemoryRegion(vaddr_t start, vaddr_t end
                                    , MemoryFlags flags
                                    , MemoryContent content
                                    , MemoryAllocationOptions type)
            : Range({start, end})
            , Flags(flags)
            , Type(type)
            , Content(content)
            , Next(nullptr)
            , Prev(nullptr)
        {

        }

        inline constexpr MemoryRegion(vaddr_t start, vaddr_t end
                                    , MemoryFlags flags
                                    , MemoryContent content
                                    , MemoryAllocationOptions type
                                    , MemoryRegion * prev, MemoryRegion * next)
            : Range({start, end})
            , Flags(flags)
            , Type(type)
            , Content(content)
            , Next(next)
            , Prev(prev)
        {

        }

        /*  Properties  */

        inline bool IsValid() const
        {
            return this->Range.IsValid();
        }

        inline vsize_t GetSize() const
        {
            return this->Range.GetSize();
        }

        inline size_t GetPageCount() const
        {
            return this->Range.GetPageCount();
        }

        /*  Queries  */

        inline bool Contains(vaddr_t vaddr) const
        {
            return this->Range.Contains(vaddr);
        }

        /*  Fields  */

        MemoryRange Range;

        MemoryFlags Flags;
        MemoryAllocationOptions Type;
        MemoryContent Content;

        MemoryRegion * Next, * Prev;
    };

    struct AdjacentMemoryRegion
    {
        /*  Constructors  */

        inline constexpr AdjacentMemoryRegion(MemoryRegion const payload)
            : Payload(payload)
            , Finding()
        {

        }

        /*  Fields  */

        MemoryRegion Payload;
        MemoryRegion Finding;
    };
}}
