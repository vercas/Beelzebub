/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

#include <beel/result.hpp>

/**
 *  Possible outcomes of handle table initialization.
 */
#define __ENUM_HANDLETABINITRES(ENUMINST) \
    ENUMINST(Success    , 0) \
    ENUMINST(OutOfMemory, 1)

__PUB_ENUM(HandleTableInitializationResult, __ENUM_HANDLETABINITRES, LITE, uint8_t)

/**
 *  Possible outcomes of handle allocation in the handle table.
 */
#define __ENUM_HANDLEALLOCRES(ENUMINST) \
    ENUMINST(Success    , 0) \
    ENUMINST(TableFull  , 1) \
    ENUMINST(InvalidPcid, 2)

__PUB_ENUM(HandleAllocationResult, __ENUM_HANDLEALLOCRES, LITE, uint8_t)

/**
 *  Possible outcomes of handle retrieval from the handle table.
 */
#define __ENUM_HANDLEGETRES(ENUMINST) \
    ENUMINST(Success    , 0) \
    ENUMINST(Unallocated, 1)

__PUB_ENUM(HandleGetResult, __ENUM_HANDLEGETRES, LITE, uint8_t)

namespace Beelzebub
{
    using handle_inner_t = uint32_t;

    union handle_t
    {
        handle_inner_t Value;

        __artificial explicit constexpr handle_t() : Value((handle_inner_t)(-1L)) { }
        __artificial explicit constexpr handle_t(handle_inner_t val) : Value(val) { }

        __artificial explicit operator handle_inner_t() const { return this->Value; }
    };

    #define CMP_OP(TYP, OP) __artificial constexpr bool operator OP (TYP a, TYP b) { return a.Value OP b.Value; }
    CMP_OP(handle_t, ==) CMP_OP(handle_t, !=)
    #undef CMP_OP

    struct HandleTableEntry
    {
        uint16_t ReferenceCount;
        uint16_t ProcessId;
        handle_t LocalIndex;
    };

    /**
     *  An interface to the kernel's handle table.
     */
    class HandleTable
    {
    public:
        /*  Statics  */

        static constexpr handle_t const InvalidHandle = handle_t();

    protected:
        /*  Constructor(s)  */

        HandleTable() = default;

    public:
        HandleTable(HandleTable const &) = delete;
        HandleTable & operator =(HandleTable const &) = delete;

        /*  Initialization  */

        static __startup HandleTableInitializationResult Initialize(handle_t limit);

        /*  Operation  */

        static __hot Result<HandleAllocationResult, handle_t> Allocate(uint16_t pcid);
        static __hot bool Deallocate(handle_t ind);
        static __hot Result<HandleGetResult, HandleTableEntry> Get(handle_t ind);

        /*  Properties  */

        static handle_t GetMaximum();
    };
}
