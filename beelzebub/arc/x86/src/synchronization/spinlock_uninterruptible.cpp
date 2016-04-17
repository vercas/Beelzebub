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

#include <synchronization/spinlock_uninterruptible.hpp>
#include <debug.hpp>

using namespace Beelzebub::Synchronization;

/*************************************
    SpinlockUninterruptible struct
*************************************/

#ifdef __BEELZEBUB__DEBUG
    /*  Destructor  */

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    SpinlockUninterruptible<false>::~SpinlockUninterruptible()
    #else
    template<bool SMP>
    SpinlockUninterruptible<SMP>::~SpinlockUninterruptible()
    #endif
    {
        assert(this->Check(), "Spinlock (uninterruptible) @ %Xp was destructed while busy!", this);

        //this->Release();
    }//*/
#endif

#ifdef __BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS
    /*  Operations  */

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    bool SpinlockUninterruptible<false>::TryAcquire(int_cookie_t & cookie) volatile
    #else
    template<bool SMP>
    bool SpinlockUninterruptible<SMP>::TryAcquire(int_cookie_t & cookie) volatile
    #endif
    {
        cookie = System::Interrupts::PushDisable();

        uint16_t const oldTail = this->Value.Tail;
        spinlock_t cmp {oldTail, oldTail};
        spinlock_t const newVal {oldTail, (uint16_t)(oldTail + 1)};
        spinlock_t const cmpCpy = cmp;

        asm volatile( "lock cmpxchgl %[newVal], %[curVal] \n\t"
                    : [curVal]"+m"(this->Value), "+a"(cmp)
                    : [newVal]"r"(newVal)
                    : "cc" );

        if likely(cmp.Overall == cmpCpy.Overall)
            return true;
        
        System::Interrupts::RestoreState(cookie);
        //  If the spinlock was already locked, restore interrupt state.

        return false;
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    void SpinlockUninterruptible<false>::Spin() const volatile
    #else
    template<bool SMP>
    void SpinlockUninterruptible<SMP>::Spin() const volatile
    #endif
    {
        spinlock_t copy;

        do
        {
            copy = {this->Value.Overall};

            asm volatile ( "pause \n\t" : : : "memory" );
        } while (copy.Tail != copy.Head);
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    void SpinlockUninterruptible<false>::Await() const volatile
    #else
    template<bool SMP>
    void SpinlockUninterruptible<SMP>::Await() const volatile
    #endif
    {
        spinlock_t copy = {this->Value.Overall};

        while (copy.Tail != copy.Head)
        {
            asm volatile ( "pause \n\t" : : : "memory" );

            copy = {this->Value.Overall};
        }
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    int_cookie_t SpinlockUninterruptible<false>::Acquire() volatile
    #else
    template<bool SMP>
    int_cookie_t SpinlockUninterruptible<SMP>::Acquire() volatile
    #endif
    {
        uint16_t myTicket = 1;

        int_cookie_t const cookie = System::Interrupts::PushDisable();

        asm volatile( "lock xaddw %[ticket], %[tail] \n\t"
                    : [tail]"+m"(this->Value.Tail)
                    , [ticket]"+r"(myTicket)
                    : : "cc" );
        //  It's possible to address the upper word directly.

        while (this->Value.Head != myTicket)
            asm volatile ( "pause \n\t" : : : "memory" );

        return cookie;
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    void SpinlockUninterruptible<false>::SimplyAcquire() volatile
    #else
    template<bool SMP>
    void SpinlockUninterruptible<SMP>::SimplyAcquire() volatile
    #endif
    {
        uint16_t myTicket = 1;

        asm volatile( "lock xaddw %[ticket], %[tail] \n\t"
                    : [tail]"+m"(this->Value.Tail)
                    , [ticket]"+r"(myTicket)
                    : : "cc" );
        //  It's possible to address the upper word directly.

        while (this->Value.Head != myTicket)
            asm volatile ( "pause \n\t" : : : "memory" );
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    void SpinlockUninterruptible<false>::Release(int_cookie_t const cookie) volatile
    #else
    template<bool SMP>
    void SpinlockUninterruptible<SMP>::Release(int_cookie_t const cookie) volatile
    #endif
    {
        asm volatile( "lock addw $1, %[head] \n\t"
                    : [head]"+m"(this->Value.Head)
                    : : "cc" );

        System::Interrupts::RestoreState(cookie);
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    void SpinlockUninterruptible<false>::SimplyRelease() volatile
    #else
    template<bool SMP>
    void SpinlockUninterruptible<SMP>::SimplyRelease() volatile
    #endif
    {
        asm volatile( "lock addw $1, %[head] \n\t"
                    : [head]"+m"(this->Value.Head)
                    : : "cc" );
    }

    #if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
    bool SpinlockUninterruptible<false>::Check() const volatile
    #else
    template<bool SMP>
    bool SpinlockUninterruptible<SMP>::Check() const volatile
    #endif
    {
        spinlock_t copy = {this->Value.Overall};

        return copy.Head == copy.Tail;
    }
#endif

namespace Beelzebub { namespace Synchronization
{
    template struct SpinlockUninterruptible<true>;
    template struct SpinlockUninterruptible<false>;
}}
