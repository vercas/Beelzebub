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

    __bland void MiscellaneousInterruptHandler(INTERRUPT_HANDLER_ARGS);

    __bland void DivideErrorHandler(INTERRUPT_HANDLER_ARGS);
    __bland void OverflowHandler(INTERRUPT_HANDLER_ARGS);
    __bland void BoundRangeExceededHandler(INTERRUPT_HANDLER_ARGS);
    __bland void InvalidOpcodeHandler(INTERRUPT_HANDLER_ARGS);
    __bland void DoubleFaultHandler(INTERRUPT_HANDLER_ARGS);
    __bland void InvalidTssHandler(INTERRUPT_HANDLER_ARGS);
    __bland void SegmentNotPresentHandler(INTERRUPT_HANDLER_ARGS);
    __bland void StackSegmentFaultHandler(INTERRUPT_HANDLER_ARGS);
    __bland void GeneralProtectionHandler(INTERRUPT_HANDLER_ARGS);
    __hot __bland void PageFaultHandler(INTERRUPT_HANDLER_ARGS);
}}
