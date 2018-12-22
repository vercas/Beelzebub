/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

/*  Note that the implementation of this header is architecture-specific.  */

#pragma once

#include <beel/structs.kernel.h>
#include "execution/thread.hpp"
#include "execution/process.hpp"

namespace Beelzebub
{
    /**
     *  <summary>Represents an abstraction of the system's IRQs and ISRs.</summary>
     */
    class Scheduler
    {
    public:
        /*  Public  */

        static __thread tid_t IdleTid;

    protected:
        /*  Constructor(s)  */

        Scheduler() = default;

    public:
        Scheduler(Scheduler const &) = delete;
        Scheduler & operator =(Scheduler const &) = delete;

        /*  Initialization  */

        static __startup Handle Initialize(bool bsp);

        static __cold void Engage();

        /*  Properties  */

        static size_t GetMaximumProcesses();
        static size_t GetMaximumThreads();
        static size_t GetMaximumScheduledThreads();

        static size_t GetIdleCores();
        static size_t GetTotalCores();
    };
}
