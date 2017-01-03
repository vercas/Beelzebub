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

#include <beel/metaprogramming.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __BEELZEBUB_TICKETLOCK_T
#define __BEELZEBUB_TICKETLOCK_T
    typedef union ticketlock_t
    {
        uint32_t volatile Overall;
        struct
        {
            uint16_t volatile Head;
            uint16_t volatile Tail;
        } HT;
    } Spinlock;
#endif

/**
 *  Acquire the spinlock, if possible.
 */
__forceinline __must_check bool SpinlockTryAcquire(union ticketlock_t * const lock)
{
    COMPILER_MEMORY_BARRIER();

op_start:
    asm volatile ( "" );   //  Complete dummy.

    uint16_t const oldTail = lock->HT.Tail;
    union ticketlock_t cmp = { .HT = {oldTail, oldTail} };
    union ticketlock_t const newVal = { .HT = {oldTail, (uint16_t)(oldTail + 1)} };
    union ticketlock_t const cmpCpy = cmp;

    asm volatile( "lock cmpxchgl %[newVal], %[curVal] \n\t"
                : [curVal]"+m"(lock->Overall), "+a"(cmp)
                : [newVal]"r"(newVal)
                : "cc" );

    if (cmp.Overall != cmpCpy.Overall)
        return false;
op_end:

    COMPILER_MEMORY_BARRIER();
    ANNOTATE_LOCK_OPERATION_CHK;

    return true;
}

/**
 *  Awaits for the spinlock to be freed.
 *  Does not acquire the lock.
 */
__forceinline void SpinlockSpin(union ticketlock_t * const lock)
{
    COMPILER_MEMORY_BARRIER();

    union ticketlock_t copy;

op_start:
    do
    {
        copy.Overall = lock->Overall;

        asm volatile ( "pause \n\t" : : : "memory" );
    } while (copy.HT.Tail != copy.HT.Head);
op_end:

    COMPILER_MEMORY_BARRIER();
    ANNOTATE_LOCK_OPERATION_CHK;
}

/**
 *  Checks if the spinlock is free. If not, it awaits.
 *  Does not acquire the lock.
 */
__forceinline void SpinlockAwait(union ticketlock_t * const lock)
{
    union ticketlock_t copy;

    COMPILER_MEMORY_BARRIER();

op_start:
    copy = *lock;

    while (copy.HT.Tail != copy.HT.Head)
    {
        asm volatile ( "pause \n\t" : : : "memory" );

        copy.Overall = lock->Overall;
    }
op_end:

    COMPILER_MEMORY_BARRIER();
    ANNOTATE_LOCK_OPERATION_CHK;
}

/**
 *  Acquire the spinlock, waiting if necessary.
 */
__forceinline void SpinlockAcquire(union ticketlock_t * const lock)
{
    uint16_t myTicket;

    COMPILER_MEMORY_BARRIER();

op_start:
    myTicket = 1;

    asm volatile( "lock xaddw %[ticket], %[tail] \n\t"
                : [tail]"+m"(lock->HT.Tail)
                , [ticket]"+r"(myTicket)
                : : "cc" );
    //  It's possible to address the upper word directly.

    while (lock->HT.Head != myTicket)
        asm volatile ( "pause \n\t" : : : "memory" );
op_end:

    COMPILER_MEMORY_BARRIER();
    ANNOTATE_LOCK_OPERATION_CHK;
}

/**
 *  Release the spinlock.
 */
__forceinline void SpinlockRelease(union ticketlock_t * const lock)
{
    COMPILER_MEMORY_BARRIER();

op_start:
    asm volatile( "lock addw $1, %[head] \n\t"
                : [head]"+m"(lock->HT.Head)
                : : "cc" );
op_end:

    COMPILER_MEMORY_BARRIER();
    ANNOTATE_LOCK_OPERATION_CHK;
}

/**
 *  Checks whether the spinlock is free or not.
 */
__forceinline __must_check bool SpinlockCheck(union ticketlock_t * const lock)
{
    union ticketlock_t copy;

    COMPILER_MEMORY_BARRIER();

op_start:
    copy = *lock;

    if (copy.HT.Head != copy.HT.Tail)
        return false;
op_end:

    COMPILER_MEMORY_BARRIER();
    ANNOTATE_LOCK_OPERATION_CHK;

    return true;
}

/**
 *  Reset the spinlock.
 */
__forceinline void SpinlockReset(union ticketlock_t * const lock)
{
    COMPILER_MEMORY_BARRIER();

    lock->Overall = 0;

    COMPILER_MEMORY_BARRIER();
}

#ifdef __cplusplus
}
#endif
