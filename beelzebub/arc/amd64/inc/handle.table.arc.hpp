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

#pragma once

#include "handle.table.hpp"

namespace Beelzebub
{
    struct HandleReferenceNode
    {
        HandleReferenceNode * Next;
        handle_t LocalIndex;
        uint16_t ProcessId;
    };

    struct HandleTableEntryArc : HandleTableEntry
    {
        HandleReferenceNode * ReferenceList;

        inline HandleTableEntryArc(uint16_t refcnt, uint16_t pcid, handle_t lind)
            : HandleTableEntry { refcnt, pcid, lind }
            , ReferenceList(nullptr)
        {

        }
    };

    /**
     *  An architecture-specific interface to the kernel's handle table.
     */
    class HandleTableArc
    {
    public:
        /*  Statics  */

        static HandleTableEntryArc * Table;
        static handle_t GlobalFreeIndex;
        static handle_t Maximum, Cursor;

        static handle_t FreeListThreshold;
        static handle_t FreeListRemovalCount;

    protected:
        /*  Constructor(s)  */

        HandleTableArc() = default;

    public:
        HandleTableArc(HandleTableArc const &) = delete;
        HandleTableArc & operator =(HandleTableArc const &) = delete;

        /*  Garbage Collection  */

        static __startup void CollectLocalFreeList(handle_t count = FreeListRemovalCount);
    };
}
