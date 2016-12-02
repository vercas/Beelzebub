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

#ifdef __BEELZEBUB__TEST_METAP

#include <tests/meta.hpp>
#include <synchronization/spinlock.hpp>
#include <synchronization/spinlock_uninterruptible.hpp>
#include <beel/interrupt.state.hpp>

#include <debug.hpp>

#define __uninterrupted                     \
    void const * _int_cookie;               \
    asm volatile ( "pushf      \n\t"        \
                   "pop %[dst] \n\t"        \
                   "cli        \n\t"        \
                 : [dst]"=r"(_int_cookie)   \
                 :                          \
                 : "memory")
#define __restore_interrupts                \
    asm volatile ( "push %[src] \n\t"       \
                   "popf        \n\t"       \
                 :                          \
                 : [src]"rm"(_int_cookie)   \
                 : "memory", "cc" )

#define __lock_guard(lock) struct MCATS(_LockGuard_, __LINE__)                                  \
{   inline MCATS(_LockGuard_, __LINE__)() : Cookie(lock.Acquire()) { }                          \
    MCATS(_LockGuard_, __LINE__)(MCATS(_LockGuard_, __LINE__) const &) = delete;                \
    MCATS(_LockGuard_, __LINE__) & operator =(MCATS(_LockGuard_, __LINE__) const &) = delete;   \
    MCATS(_LockGuard_, __LINE__) & operator =(MCATS(_LockGuard_, __LINE__) &&) = delete;        \
    inline ~MCATS(_LockGuard_, __LINE__)() { lock.Release(this->Cookie); }                      \
    private: decltype(lock)::Cookie const Cookie;                                               \
} MCATS(_lock_guard_, __LINE__)

#define __lock_guard_simple(lock) struct MCATS(_LockGuard_, __LINE__)                           \
{   inline MCATS(_LockGuard_, __LINE__)() { lock.SimplyAcquire(); }                             \
    MCATS(_LockGuard_, __LINE__)(MCATS(_LockGuard_, __LINE__) const &) = delete;                \
    MCATS(_LockGuard_, __LINE__) & operator =(MCATS(_LockGuard_, __LINE__) const &) = delete;   \
    MCATS(_LockGuard_, __LINE__) & operator =(MCATS(_LockGuard_, __LINE__) &&) = delete;        \
    inline ~MCATS(_LockGuard_, __LINE__)() { lock.SimplyRelease(); }                            \
} MCATS(_lock_guard_, __LINE__)

#define withLockStatic(lock) with(__lock_guard(lock))
#define withLockSimple(lock) with(__lock_guard_simple(lock))

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

SpinlockUninterruptible<false> TestLock1;
Spinlock<false> TestLock2;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

__cold __noinline void TestMetaprogramming1()
{
    InterruptState const cookie = InterruptState::Disable();

    bool volatile rada = true;

    cookie.Restore();
}

__cold __noinline void TestMetaprogramming2()
{
    InterruptGuard<> intGuard;

    bool volatile rada = true;
}

__cold __noinline void TestMetaprogramming3()
{
    __uninterrupted;

    bool volatile rada = true;

    __restore_interrupts;
}

__cold __noinline void TestMetaprogramming4()
{
    withInterrupts (false)
    {
        bool volatile rada = true;
    }
}

__cold __noinline void TestMetaprogramming5()
{
    withInterrupts (false)
    {
        bool volatile rada = true;
    }

    withInterrupts (false)
    {
        bool volatile rada = true;
    }
}

__cold __noinline void TestMetaprogramming6()
{
    withLock (TestLock2)
    {
        bool volatile rada = true;
    }
}

__cold __noinline void TestMetaprogramming7()
{
    withLockStatic (TestLock1)
    {
        bool volatile rada = true;
    }
}

__cold __noinline void TestMetaprogramming8()
{
    withLock (TestLock1)
    {
        bool volatile rada = true;
    }
}

#pragma GCC diagnostic pop

void TestMetaprogramming()
{
    TestMetaprogramming1();
    TestMetaprogramming2();
    TestMetaprogramming3();
    TestMetaprogramming4();
    TestMetaprogramming5();
    TestMetaprogramming6();
    TestMetaprogramming7();
    TestMetaprogramming8();
}

#endif
