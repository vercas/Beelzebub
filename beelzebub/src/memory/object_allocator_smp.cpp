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

#if   defined(__BEELZEBUB_SETTINGS_SMP)
//  Since this file only implements stuff, there's nothing to do without SMP
//  support.

    #include <memory/object_allocator_smp.hpp>

    #include <math.h>
    #include <debug.hpp>

    using namespace Beelzebub;
    using namespace Beelzebub::Memory;

    #if defined(__BEELZEBUB_KERNEL)
        #define OBJA_LOCK_TYPE Beelzebub::Synchronization::SpinlockUninterruptible<>

        #define OBJA_COOK_TYPE int_cookie_t
    #else
        //  To do: userland will require a mutex here.
    #endif

    #define OBJA_POOL_TYPE      ObjectPoolSmp
    #define OBJA_ALOC_TYPE      ObjectAllocatorSmp
    #define OBJA_MULTICONSUMER  true
    #include <memory/object_allocator_cbase.inc>
    #undef OBJA_MULTICONSUMER
    #undef OBJA_ALOC_TYPE
    #undef OBJA_POOL_TYPE

    #undef OBJA_COOK_TYPE
    #undef OBJA_LOCK_TYPE

#endif
