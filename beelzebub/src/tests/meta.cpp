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
#include <synchronization/lock_guard.hpp>
#include <system/interrupts.hpp>

#include <debug.hpp>

#define __uninterrupted                     \
    int_cookie_t _int_cookie;               \
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

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

void TestMetaprogramming1()
{
    int_cookie_t const cookie = Interrupts::PushDisable();

    bool volatile rada = true;

    Interrupts::RestoreState(cookie);
}

void TestMetaprogramming2()
{
    InterruptGuard intGuard;

    bool volatile rada = true;
}

void TestMetaprogramming3()
{
    __uninterrupted;

    bool volatile rada = true;

    __restore_interrupts;
}

void TestMetaprogramming4()
{
    //uninterrupted
    {
        bool volatile rada = true;
    }
}

#endif
