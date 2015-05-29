#pragma once

#include <kernel.hpp>
#include <handles.h>
#include <debug.hpp>
#include <isr.h>
#include <metaprogramming.h>

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

    __bland void MiscellaneousInterruptHandler(IsrState * const state);

    __bland void DivideErrorHandler(IsrState * const state);
    __bland void OverflowHandler(IsrState * const state);
    __bland void BoundRangeExceededHandler(IsrState * const state);
    __bland void InvalidOpcodeHandler(IsrState * const state);
    __bland void DoubleFaultHandler(IsrState * const state);
    __bland void InvalidTssHandler(IsrState * const state);
    __bland void SegmentNotPresentHandler(IsrState * const state);
    __bland void StackSegmentFaultHandler(IsrState * const state);
    __bland void GeneralProtectionHandler(IsrState * const state);
    __bland void PageFaultHandler(IsrState * const state);
}}
