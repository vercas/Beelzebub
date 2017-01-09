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

#include <beel/sync/ticket.lock.unint.hpp>
#include <debug.hpp>

using namespace Beelzebub::Synchronization;

/***************************************
    TicketLockUninterruptible struct
***************************************/

#ifdef __BEELZEBUB__CONF_DEBUG
    /*  Destructor  */

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    TicketLockUninterruptible<false>::~TicketLockUninterruptible()
    #else
    template<bool SMP>
    TicketLockUninterruptible<SMP>::~TicketLockUninterruptible()
    #endif
    {
        assert(this->Check(), "TicketLock (uninterruptible) @ %Xp was destructed while busy!", this);

        //this->Release();
    }//*/
#endif

#ifdef __BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS
    /*  Operations  */

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    bool TicketLockUninterruptible<false>::TryAcquire(InterruptState & cookie) volatile
    #else
    template<bool SMP>
    bool TicketLockUninterruptible<SMP>::TryAcquire(InterruptState & cookie) volatile
    #endif
    {
        cookie = InterruptState::Disable();

        uint16_t const oldTail = this->Value.Tail;
        ticketlock_t cmp {oldTail, oldTail};
        ticketlock_t const newVal {oldTail, (uint16_t)(oldTail + 1)};
        ticketlock_t const cmpCpy = cmp;

        asm volatile( "lock cmpxchgl %[newVal], %[curVal] \n\t"
                    : [curVal]"+m"(this->Value), "+a"(cmp)
                    : [newVal]"r"(newVal)
                    : "cc" );

        if likely(cmp.Overall == cmpCpy.Overall)
            return true;
        
        cookie.Restore();
        //  If the spinlock was already locked, restore interrupt state.

        return false;
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    void TicketLockUninterruptible<false>::Spin() const volatile
    #else
    template<bool SMP>
    void TicketLockUninterruptible<SMP>::Spin() const volatile
    #endif
    {
        ticketlock_t copy;

        do
        {
            copy = {this->Value.Overall};

            asm volatile ( "pause \n\t" );
        } while (copy.Tail != copy.Head);
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    void TicketLockUninterruptible<false>::Await() const volatile
    #else
    template<bool SMP>
    void TicketLockUninterruptible<SMP>::Await() const volatile
    #endif
    {
        ticketlock_t copy = {this->Value.Overall};

        while (copy.Tail != copy.Head)
        {
            asm volatile ( "pause \n\t" );

            copy = {this->Value.Overall};
        }
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    InterruptState TicketLockUninterruptible<false>::Acquire() volatile
    #else
    template<bool SMP>
    InterruptState TicketLockUninterruptible<SMP>::Acquire() volatile
    #endif
    {
        uint16_t myTicket = 1;

        InterruptState const cookie = InterruptState::Disable();

        asm volatile( "lock xaddw %[ticket], %[tail] \n\t"
                    : [tail]"+m"(this->Value.Tail)
                    , [ticket]"+r"(myTicket)
                    : : "cc" );
        //  It's possible to address the upper word directly.

        uint16_t diff;

        while ((diff = myTicket - this->Value.Head) != 0)
            do asm volatile ( "pause \n\t" ); while (--diff != 0);

        return cookie;
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    void TicketLockUninterruptible<false>::SimplyAcquire() volatile
    #else
    template<bool SMP>
    void TicketLockUninterruptible<SMP>::SimplyAcquire() volatile
    #endif
    {
        uint16_t myTicket = 1;

        asm volatile( "lock xaddw %[ticket], %[tail] \n\t"
                    : [tail]"+m"(this->Value.Tail)
                    , [ticket]"+r"(myTicket)
                    : : "cc" );
        //  It's possible to address the upper word directly.

        while (this->Value.Head != myTicket)
            asm volatile ( "pause \n\t" );
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    void TicketLockUninterruptible<false>::Release(InterruptState const cookie) volatile
    #else
    template<bool SMP>
    void TicketLockUninterruptible<SMP>::Release(InterruptState const cookie) volatile
    #endif
    {
        asm volatile( "lock addw $1, %[head] \n\t"
                    : [head]"+m"(this->Value.Head)
                    : : "cc" );

        cookie.Restore();
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    void TicketLockUninterruptible<false>::SimplyRelease() volatile
    #else
    template<bool SMP>
    void TicketLockUninterruptible<SMP>::SimplyRelease() volatile
    #endif
    {
        asm volatile( "lock addw $1, %[head] \n\t"
                    : [head]"+m"(this->Value.Head)
                    : : "cc" );
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    bool TicketLockUninterruptible<false>::Check() const volatile
    #else
    template<bool SMP>
    bool TicketLockUninterruptible<SMP>::Check() const volatile
    #endif
    {
        ticketlock_t copy = {this->Value.Overall};

        return copy.Head == copy.Tail;
    }
#endif

namespace Beelzebub { namespace Synchronization
{
    template struct TicketLockUninterruptible<true>;
    template struct TicketLockUninterruptible<false>;
}}
