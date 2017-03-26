/*
    Copyright (c) 2017 Alexandru-Mihai Maftei. All rights reserved.


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

#include "system/interrupts.hpp"

namespace Beelzebub { namespace System
{
    enum class BreakpointCondition : size_t
    {
        InstructionFetch = 0,
        DataWrite = 1,
        Io = 2,
        DataReadWrite = 3,
    };

    enum class BreakpointSize : size_t
    {
        One = 0, Two = 1, Eight = 2, Four = 3,
    };

    struct BreakpointProperties
    {
        BreakpointCondition Condition;
        BreakpointSize Size;
        bool Global, Local;

        inline bool IsEnabled() const { return this->Global || this->Local; }
    };

    typedef void (*BreakpointFunction)(INTERRUPT_HANDLER_ARGS_FULL, void * address, BreakpointProperties & bp);

    /**
     *  <summary>Represents an abstraction of the system's debug registers.</summary>
     */
    class DebugRegisters
    {
    protected:
        /*  Constructor(s)  */

        DebugRegisters() = default;

    public:
        DebugRegisters(DebugRegisters const &) = delete;
        DebugRegisters & operator =(DebugRegisters const &) = delete;

        /*  Initialization  */

        static __startup void Initialize();

        /*  Operation  */

        static __solid bool AddBreakpoint(void * addr, size_t size, bool global
            , BreakpointCondition bc, BreakpointFunction fnc);
        static __solid bool RemoveBreakpoint(void * addr);

        /*  Properties  */

        static size_t GetBreakpointCount();
    };
}}
