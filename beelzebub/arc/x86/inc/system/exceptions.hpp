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

#include <system/interrupts.hpp>

namespace Beelzebub { namespace System
{
    /**
     *  A few known exception vectors
     */
    enum class KnownExceptionVectors : uint8_t
    {
        //  Integer division by 0.
        DivideError = 0,
        //  No idea.
        Debug = 1,
        NmiInterrupt = 2,
        Breakpoint = 3,
        Overflow = 4,
        BoundRangeExceeded = 5,
        InvalidOpcode = 6,
        NoMathCoprocessor = 7,
        DoubleFault = 8,
        CoprocessorSegmentOverrun = 9,
        InvalidTss = 10,
        SegmentNotPresent = 11,
        StackSegmentFault = 12,
        GeneralProtectionFault = 13,
        PageFault = 14,
        Reserved1 = 15,
        FloatingPointError = 16,
        AlignmentCheck = 17,
        MachineCheck = 18,
        SimdFloatingPointException = 19,
    };

    void MiscellaneousInterruptHandler(INTERRUPT_HANDLER_ARGS);

    void DivideErrorHandler(INTERRUPT_HANDLER_ARGS);
    void OverflowHandler(INTERRUPT_HANDLER_ARGS);
    void BoundRangeExceededHandler(INTERRUPT_HANDLER_ARGS);
    void InvalidOpcodeHandler(INTERRUPT_HANDLER_ARGS);
    void DoubleFaultHandler(INTERRUPT_HANDLER_ARGS);
    void InvalidTssHandler(INTERRUPT_HANDLER_ARGS);
    void SegmentNotPresentHandler(INTERRUPT_HANDLER_ARGS);
    void StackSegmentFaultHandler(INTERRUPT_HANDLER_ARGS);
    void GeneralProtectionHandler(INTERRUPT_HANDLER_ARGS);
    __hot void PageFaultHandler(INTERRUPT_HANDLER_ARGS);
}}
