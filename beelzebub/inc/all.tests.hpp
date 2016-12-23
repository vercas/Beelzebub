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

#ifdef __BEELZEBUB__TEST_STACKINT
#include "tests/stack_integrity.hpp"
#endif

#ifdef __BEELZEBUB__TEST_METAP
#include "tests/meta.hpp"
#endif

#ifdef __BEELZEBUB__TEST_EXCP
#include "tests/exceptions.hpp"
#endif

#ifdef __BEELZEBUB__TEST_STR
#include "tests/string.hpp"
#endif

#ifdef __BEELZEBUB__TEST_TERMINAL
#include "tests/terminal.hpp"
#endif

#ifdef __BEELZEBUB__TEST_OBJA
#include "tests/object_allocator.hpp"
#endif

#ifdef __BEELZEBUB__TEST_AVL_TREE
#include "tests/avl_tree.hpp"
#endif

#ifdef __BEELZEBUB__TEST_CMDO
#include "tests/cmdo.hpp"
#endif

#ifdef __BEELZEBUB__TEST_FPU
#include "tests/fpu.hpp"
#endif

#ifdef __BEELZEBUB__TEST_BIGINT
#include "tests/bigint.hpp"
#endif

#ifdef __BEELZEBUB__TEST_LOCK_ELISION
#include "tests/lock_elision.hpp"
#endif

#ifdef __BEELZEBUB__TEST_RW_SPINLOCK
#include "tests/rw_spinlock.hpp"
#endif

#ifdef __BEELZEBUB__TEST_VAS
#include "tests/vas.hpp"
#endif

#if defined(__BEELZEBUB__TEST_MALLOC) && !defined(__BEELZEBUB_SETTINGS_KRNDYNALLOC_NONE)
#include "tests/malloc.hpp"
#endif

#ifdef __BEELZEBUB__TEST_INTERRUPT_LATENCY
#include "tests/interrupt_latency.hpp"
#endif

#ifdef __BEELZEBUB__TEST_KMOD
#include "tests/kmod.hpp"
#endif

#ifdef __BEELZEBUB__TEST_TIMER
#include "tests/timer.hpp"
#endif

#if defined(__BEELZEBUB_SETTINGS_SMP) && defined(__BEELZEBUB__TEST_MAILBOX)
#include "tests/mailbox.hpp"
#endif
