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

#ifndef _BEEL_SYNC_TICKETLOCK_UNI_H
#define _BEEL_SYNC_TICKETLOCK_UNI_H
//  Sue me.

#include <beel/interrupt.state.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __BEELZEBUB_TICKETLOCK_C_T
#define __BEELZEBUB_TICKETLOCK_C_T
    typedef union ticketlock_t
    {
        uint32_t Overall;

        struct
        {
            uint16_t Tail, Head;
        } HT;
    } TicketLock;
#endif

typedef InterruptState TicketLockUniCookie;

#if defined(__BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS) && !defined(__TICKETLOCK_SOURCE)
    /**
     *  Acquire the ticket lock, if possible.
     */
    __solid __must_check bool TicketLockUniTryAcquire(union ticketlock_t * const lock, InterruptState * const cookie);

    /**
     *  Awaits for the ticket lock to be freed.
     *  Does not acquire the lock.
     */
    __solid void TicketLockUniSpin(union ticketlock_t * const lock);

    /**
     *  Checks if the ticket lock is free. If not, it awaits.
     *  Does not acquire the lock.
     */
    __solid void TicketLockUniAwait(union ticketlock_t * const lock);

    /**
     *  Acquire the ticket lock, waiting if necessary.
     */
    __solid __must_check InterruptState TicketLockUniAcquire(union ticketlock_t * const lock);

    /**
     *  Release the ticket lock.
     */
    __solid void TicketLockUniRelease(union ticketlock_t * const lock, InterruptState const cookie);

    /**
     *  Checks whether the ticket lock is free or not.
     */
    __solid __must_check bool TicketLockUniCheck(union ticketlock_t * const lock);

    /**
     *  Reset the ticket lock.
     */
    __solid void TicketLockUniReset(union ticketlock_t * const lock);

#else

    #ifdef __TICKETLOCK_SOURCE
    #pragma push_macro("__forceinline")
    #pragma push_macro("__must_check")
    #undef __forceinline
    #undef __must_check
    #define __forceinline  
    #define __must_check
    #endif

    /**
     *  Acquire the ticket lock, if possible.
     */
    __forceinline __must_check bool TicketLockUniTryAcquire(
        union ticketlock_t * const lock, InterruptState * const cookie)
    {
        *cookie = InterruptStateDisable();

        COMPILER_MEMORY_BARRIER();

    op_start:
        EMPTY_STATEMENT;

        uint16_t const oldHead = lock->HT.Head;

        FORCE_EVAL(oldHead);

        union ticketlock_t cmp = { .HT = {oldHead, oldHead} };
        union ticketlock_t const newVal = { .HT = {oldHead, (uint16_t)(oldHead + 1)} };

        if (!__atomic_compare_exchange_n(&(lock->Overall), &(cmp.Overall), newVal.Overall, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        {
            InterruptStateRestore(*cookie);
            //  If the ticket lock was already locked, restore interrupt state.

            return false;
        }
    op_end:

        COMPILER_MEMORY_BARRIER();
        ANNOTATE_LOCK_OPERATION_ACQ;

        return true;
    }

    /**
     *  Awaits for the ticket lock to be freed.
     *  Does not acquire the lock.
     */
    __forceinline void TicketLockUniSpin(union ticketlock_t * const lock)
    {
        COMPILER_MEMORY_BARRIER();

    op_start:
        EMPTY_STATEMENT;

        union ticketlock_t copy;

        do
        {
            copy.Overall = lock->Overall;

            DO_NOTHING();
        } while (copy.HT.Tail != copy.HT.Head);
    op_end:

        COMPILER_MEMORY_BARRIER();
        ANNOTATE_LOCK_OPERATION_CHK;
    }

    /**
     *  Checks if the ticket lock is free. If not, it awaits.
     *  Does not acquire the lock.
     */
    __forceinline void TicketLockUniAwait(union ticketlock_t * const lock)
    {
        COMPILER_MEMORY_BARRIER();

    op_start:
        EMPTY_STATEMENT;

        union ticketlock_t copy = *lock;

        while (copy.HT.Tail != copy.HT.Head)
        {
            DO_NOTHING();

            copy.Overall = lock->Overall;
        }
    op_end:

        COMPILER_MEMORY_BARRIER();
        ANNOTATE_LOCK_OPERATION_CHK;
    }

    /**
     *  Acquire the ticket lock, waiting if necessary.
     */
    __forceinline __must_check InterruptState TicketLockUniAcquire(
        union ticketlock_t * const lock)
    {
        InterruptState const cookie = InterruptStateDisable();

        COMPILER_MEMORY_BARRIER();

    op_start:
        EMPTY_STATEMENT;

        uint16_t const myTicket = __atomic_fetch_add(&(lock->HT.Head), 1, __ATOMIC_ACQUIRE);

        uint16_t diff;

        while ((diff = myTicket - lock->HT.Tail) != 0)
            do DO_NOTHING(); while (--diff != 0);
    op_end:

        COMPILER_MEMORY_BARRIER();
        ANNOTATE_LOCK_OPERATION_ACQ;

        return cookie;
    }

    /**
     *  Release the ticket lock.
     */
    __forceinline void TicketLockUniRelease(
        union ticketlock_t * const lock, InterruptState const cookie)
    {
        COMPILER_MEMORY_BARRIER();

    op_start:
        ++lock->HT.Tail;
    op_end:

        COMPILER_MEMORY_BARRIER();
        ANNOTATE_LOCK_OPERATION_REL;

        InterruptStateRestore(cookie);
    }

    /**
     *  Checks whether the ticket lock is free or not.
     */
    __forceinline __must_check bool TicketLockUniCheck(union ticketlock_t * const lock)
    {
        COMPILER_MEMORY_BARRIER();

    op_start:
        EMPTY_STATEMENT;

        union ticketlock_t copy = *lock;

        if (copy.HT.Head != copy.HT.Tail)
            return false;
    op_end:

        COMPILER_MEMORY_BARRIER();
        ANNOTATE_LOCK_OPERATION_CHK;

        return true;
    }

    /**
     *  Reset the ticket lock.
     */
    __forceinline void TicketLockUniReset(union ticketlock_t * const lock)
    {
        COMPILER_MEMORY_BARRIER();

        lock->Overall = 0;

        COMPILER_MEMORY_BARRIER();
    }

    #ifdef __TICKETLOCK_SOURCE
    #pragma pop_macro("__forceinline")
    #pragma pop_macro("__must_check")
    #endif
#endif

#ifdef __cplusplus
}
#endif

#endif
