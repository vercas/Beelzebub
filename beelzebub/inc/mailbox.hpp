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
#include <beel/sync/atomic.hpp>
#include <beel/handles.h>
#include <utils/bitfields.hpp>

namespace Beelzebub
{
    typedef void (* MailFunction)(void * cookie);
    typedef void (* TimeWaster)(void * cookie);

    struct MailboxEntryBase;

    struct MailboxEntryLink
    {
        /*  Constructor(s)  */

        inline MailboxEntryLink() : Next( nullptr), Core(0) { }

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
#ifdef __BEELZEBUB_SETTINGS_MANYCORE
            size_t Generation;
#endif

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
            , Flags(0)
        {

        }

        /*  Operations  */

        void Post(TimeWaster waster = nullptr, void * cookie = nullptr, bool poll = true);

        inline void Post(bool poll, TimeWaster waster = nullptr, void * cookie = nullptr)
        {
            return this->Post(waster, cookie, poll);
        }

        /*  Properties  */

        BITFIELD_FLAG_RW(0, Await, size_t, this->Flags, , const, static)
        BITFIELD_FLAG_RW(1, NonMaskable, size_t, this->Flags, , const, static)

        /*  Fields  */

        MailFunction Function;
        void * Cookie;
        unsigned int DestinationCount; 
        Synchronization::Atomic<unsigned int> DestinationsLeft;
        size_t Flags;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        __extension__ MailboxEntryLink Links[0];
#pragma GCC diagnostic pop
    };

    static_assert(sizeof(MailboxEntryBase) == (2 * sizeof(void *) + 2 * sizeof(unsigned int) + sizeof(size_t)), "Struct size mismatch.");

    template<unsigned int N>
    struct MailboxEntry : public MailboxEntryBase
    {
        /*  Constructor(s)  */

        inline MailboxEntry(MailFunction func, void * cookie = nullptr)
            : MailboxEntryBase(N, func, cookie)
            , Destinations()
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

        static constexpr uint32_t const Broadcast = UINT32_MAX;

    protected:
        /*  Constructor(s)  */

        Mailbox() = default;

    public:
        Mailbox(Mailbox const &) = delete;
        Mailbox & operator =(Mailbox const &) = delete;

        /*  Initialization  */

        static __startup void Initialize();

        static __hot bool IsReady();

        /*  Operation  */

        // static MailboxEntryBase * GetLocalEntry();

        static __solid void Post(MailboxEntryBase * entry, TimeWaster waster = nullptr, void * cookie = nullptr, bool poll = true);

        static inline void Post(MailboxEntryBase * entry, bool poll, TimeWaster waster = nullptr, void * cookie = nullptr)
        {
            return Post(entry, waster, cookie, poll);
        }
    };

#define ALLOCATE_MAIL_4(name, dstcnt, func, cookie) \
    __extension__ void * MCATS(__, name, _buff)[(sizeof(Beelzebub::MailboxEntryBase) + (dstcnt) * sizeof(Beelzebub::MailboxEntryLink) + sizeof(void *) - 1) / sizeof(void *)]; \
    Beelzebub::MailboxEntryBase & name = *(new (reinterpret_cast<Beelzebub::MailboxEntryBase *>(&(MCATS(__, name, _buff)[0]))) Beelzebub::MailboxEntryBase((dstcnt), (func), (cookie)));
    // Beelzebub::MailboxEntryBase & name = *(new (Beelzebub::Mailbox::GetLocalEntry()) Beelzebub::MailboxEntryBase((dstcnt), (func), (cookie)));;
#define ALLOCATE_MAIL_3(name, dstcnt, func) ALLOCATE_MAIL_4(name, dstcnt, func, nullptr)
#define ALLOCATE_MAIL_2(name, dstcnt) ALLOCATE_MAIL_3(name, dstcnt, nullptr)
#define ALLOCATE_MAIL(name, ...) GET_MACRO3(__VA_ARGS__, ALLOCATE_MAIL_4, ALLOCATE_MAIL_3, ALLOCATE_MAIL_2)(name, __VA_ARGS__)

#define ALLOCATE_MAIL_BROADCAST_3(name, func, cookie) \
    ALLOCATE_MAIL_4(name, 1, func, cookie) \
    name.Links[0] = Beelzebub::MailboxEntryLink(Beelzebub::Mailbox::Broadcast);
#define ALLOCATE_MAIL_BROADCAST_2(name, func) ALLOCATE_MAIL_BROADCAST_3(name, func, nullptr)
#define ALLOCATE_MAIL_BROADCAST_1(name) ALLOCATE_MAIL_BROADCAST_2(name, nullptr)
#define ALLOCATE_MAIL_BROADCAST(...) GET_MACRO3(__VA_ARGS__, ALLOCATE_MAIL_BROADCAST_3, ALLOCATE_MAIL_BROADCAST_2, ALLOCATE_MAIL_BROADCAST_1)(__VA_ARGS__)
}
