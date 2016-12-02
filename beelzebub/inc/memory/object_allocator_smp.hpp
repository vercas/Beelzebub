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

#pragma once

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    //  The really is no point in doing all the expensive locking on non-SMP
    //  systems.

    #if   defined(__BEELZEBUB__TEST_OBJA)
        #define private public
        #define protected public
    #endif

    #include <memory/object_allocator_pools.hpp>
    #include <synchronization/spinlock.hpp>
    #include <synchronization/atomic.hpp>

    #define OBJA_LOCK_TYPE Beelzebub::Synchronization::Spinlock<>
    #define OBJA_COOK_TYPE Beelzebub::InterruptState

    namespace Beelzebub { namespace Memory
    {
        /*  First, the normal SMP-aware object allocator.  */
        #define OBJA_POOL_TYPE      ObjectPoolSmp
        #define OBJA_ALOC_TYPE      ObjectAllocatorSmp
        #define OBJA_MULTICONSUMER  true
        #define OBJA_UNINTERRUPTED  true
        #include <memory/object_allocator_hbase.inc>
        #undef OBJA_UNINTERRUPTED
        #undef OBJA_MULTICONSUMER
        #undef OBJA_ALOC_TYPE
        #undef OBJA_POOL_TYPE
    }}

    #undef OBJA_COOK_TYPE
    #undef OBJA_LOCK_TYPE

    #if   defined(__BEELZEBUB__TEST_OBJA)
        #undef private
        #undef protected
    #endif
#else
    #include <memory/object_allocator.hpp>

    namespace Beelzebub { namespace Memory
    {
        /*  Yep, an alias is all that is required.  */
        typedef ObjectAllocator ObjectAllocatorSmp;
    }}
#endif
