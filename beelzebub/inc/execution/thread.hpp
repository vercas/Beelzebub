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

#include <execution/process.hpp>
#include <execution/thread_state.hpp>

namespace Beelzebub { namespace Execution
{
    typedef void * (*ThreadEntryPointFunction)(void * const arg);

    /**
     *  A unit of execution.
     */
    class Thread
    {
    public:

        /*  Constructors  */

        inline Thread()
            : KernelStackTop()
            , KernelStackBottom()
            , KernelStackPointer()
            , State()
            , Previous()
            , Next()
            , EntryPoint()
            , Owner(nullptr) 
        {

        }

        Thread(Thread const &) = delete;
        Thread & operator =(Thread const &) = delete;

        inline Thread(Process * const owner)
            : KernelStackTop()
            , KernelStackBottom()
            , KernelStackPointer()
            , State()
            , Previous()
            , Next()
            , EntryPoint()
            , Owner(owner) 
        {

        }

        /*  Operations  */

        __hot Handle SwitchTo(Thread * const other, ThreadState * const dest);    //  Implemented in architecture-specific code.
        Handle SwitchToNext(ThreadState * const dest) { return this->SwitchTo(this->Next, dest); }

        /*  Stack  */

        uintptr_t KernelStackTop;
        uintptr_t KernelStackBottom;

        uintptr_t KernelStackPointer;

        /*  State  */

        ThreadState State;

        /*  Linkage  */

        Thread * Previous;
        Thread * Next;

        Handle IntroduceNext(Thread * const other);

        //  TODO: Eventually implement a proper scheduler and drop the linkage system.

        /*  Parameters  */

        ThreadEntryPointFunction EntryPoint;

        /*  Hierarchy  */

        Process * const Owner;
    };
}}
