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

#include <beel/timing.hpp>
#include <system/interrupts.hpp>
#include <synchronization/atomic.hpp>
#include <beel/handles.h>

namespace Beelzebub
{
    typedef void (* MailFunction)(void * cookie);

    struct MailboxEntryBase;

    struct MailboxEntryLink
    {
        /*  Constructor(s)  */

        inline MailboxEntryLink(uint32_t core)
            : Next( nullptr)
            , Core(core)
        {

        }

        /*  Fields  */

        MailboxEntryBase * Next;

        union
        {
            uint32_t Core;

            uintptr_t Padding;
        };
    };

    struct MailboxEntryBase
    {
        /*  Constructor(s)  */

        inline MailboxEntryBase(unsigned int destCnt, MailFunction func, void * cookie)
            : Function( func)
            , Cookie(cookie)
            , DestinationCount(destCnt)
            , DestinationsLeft({destCnt})
        {

        }

        /*  Fields  */

        MailFunction Function;
        void * Cookie;
        unsigned int DestinationCount; 
        Synchronization::Atomic<unsigned int> DestinationsLeft; 
        MailboxEntryLink Dests[0];
    };

    static_assert(sizeof(MailboxEntryBase) == (2 * sizeof(void *) + 2 * sizeof(unsigned int)), "Struct size mismatch.");

    template<unsigned int N>
    struct MailboxEntry : public MailboxEntryBase
    {
        /*  Constructor(s)  */

        inline MailboxEntry(MailFunction func, void * cookie = nullptr)
            : MailboxEntryBase(N, func, cookie)
        {

        }

        /*  Fields  */

        MailboxEntryLink Destinations[N];
    };

    /**
     *  <summary>Represents an abstract system mailbox.</summary>
     */
    class Mailbox
    {
    public:
        /*  Statics  */

        static constexpr size_t const LocalCount = 16;

        static constexpr size_t const Broadcast = SIZE_MAX;

    protected:
        /*  Constructor(s)  */

        Mailbox() = default;

    public:
        Mailbox(Mailbox const &) = delete;
        Mailbox & operator =(Mailbox const &) = delete;

        /*  Initialization  */

        static __startup void Initialize();

        /*  Operation  */

        template<unsigned int N>
        static void Post(MailboxEntry<N> * entry, bool poll = true)
        {
            return PostInternal(static_cast<MailboxEntryBase *>(entry), N, poll);
        }

    private:
        static void PostInternal(MailboxEntryBase * entry, unsigned int N, bool poll);
    };
}
