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

#include "execution/process.arc.hpp"
#include "memory/vas.hpp"

#include <beel/structs.kernel.h>
#include <beel/sync/smp.lock.hpp>
#include <beel/sync/atomic.hpp>
#include <beel/memory/reference.counting.hpp>

namespace Beelzebub { namespace Execution
{
    enum class ProcessState
    {
        Constructing,
        Active,
    };

    /**
     *  A unit of isolation.
     */
    class Process : public Memory::ReferenceCounted<Process>
                  , public ProcessBase
                  , public ProcessArchitecturalBase
    {
    public:
        /*  Constructors  */

        inline Process(uint16_t id = 0)
            : ReferenceCounted()
            , ProcessBase( id)
            , ProcessArchitecturalBase()
            , State(ProcessState::Constructing)
            , Name(nullptr)
            , ActiveCoreCount(0)
            , LocalTablesLock()
            , AlienPagingTablesLock()
            , Vas()
            , RuntimeLoaded(false)
        {

        }

        Process(Process const &) = delete;
        Process & operator =(Process const &) = delete;

        /*  Operations  */

        ProcessState State;
        void SetActive();

        char const * Name;
        void SetName(char const * name);

        Synchronization::Atomic<size_t> ActiveCoreCount;
        __hot Handle SwitchTo(Process * const other);

        /*  Memory  */

        Synchronization::SmpLock LocalTablesLock;

        Synchronization::SmpLock AlienPagingTablesLock;

        Memory::Vas Vas;

        bool RuntimeLoaded;
    };
}}
