/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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

#include <terminals/base.hpp>

namespace Beelzebub { namespace Terminals
{
    /**
     *  <summary>Represents an area of memory where characters are stored.</summary>
     */
    struct CharPool
    {
        /*  Constructors  */

        inline CharPool()
            : Size( 0)
            , Capacity(0)
            , Next(nullptr)
        {

        }

        inline CharPool(uint32_t const cap)
            : Size( 0)
            , Capacity(cap)
            , Next(nullptr)
        {

        }

        CharPool(CharPool const &) = delete;
        CharPool & operator =(const CharPool &) = delete;
        //  No copying.

        /*  Utilities  */

        inline char const * GetString() const
        {
            return reinterpret_cast<char const *>(this) + sizeof(CharPool);
        }

        /*  Fields  */

        uint32_t Size;
        uint32_t Capacity;

        CharPool * Next;
    };

    typedef Handle (*AcquireCharPoolFunc)(size_t minSize, size_t headerSize, CharPool * & result);
    typedef Handle (*EnlargeCharPoolFunc)(size_t minSize, size_t headerSize, CharPool * pool);
    typedef Handle (*ReleaseCharPoolFunc)(size_t headerSize, CharPool * pool);

    /**
     *  <summary>A terminal which caches output in memory.</summary>
     */
    class CacheTerminal : public TerminalBase
    {
        /*  Constructors  */

    public:
        inline CacheTerminal()
            : TerminalBase( nullptr )
            , AcquirePool(nullptr)
            , EnlargePool(nullptr)
            , ReleasePool(nullptr)
            , FirstPool(nullptr)
            , Capacity(0)
            , Count(0)
            , FreeCount(0)
        {

        }

        CacheTerminal(AcquireCharPoolFunc const acquire
                    , EnlargeCharPoolFunc const enlarge
                    , ReleaseCharPoolFunc const release);

        /*  Writing  */

    private:
        __internal TerminalWriteResult InternalWrite(char const * const str);

    public:
        virtual TerminalWriteResult WriteUtf8(char const * const c) override;
        virtual TerminalWriteResult Write(char const * const str) override;

        /*  Flushing & Cleanup  */

        TerminalWriteResult Dump(TerminalBase & target) const;
        void Clear();

        Handle Destroy();

        /*  Parameters  */

        AcquireCharPoolFunc AcquirePool;    //  When this is null, the cache
        EnlargeCharPoolFunc EnlargePool;    //  terminal is invalid.
        ReleaseCharPoolFunc ReleasePool;

        /*  Fields  */

        CharPool * FirstPool;

        size_t Capacity, Count, FreeCount;
    };
}}
