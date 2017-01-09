#include <_print/isr.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

/*****************************
    IsrStatePartial Struct
*****************************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, IsrStatePartial const * const val)
{
    return term->WriteFormat("ISR state %Xp:%n"
        "\tStack Segment: %X2%n"
        "\tData Segment: %X2%n"
        "\tCode Segment: %X2%n"
        "\tStack Pointer: %Xp%n"
        "\tInstruction Pointer: %Xp%n"
        "\tFlags: %Xs%n"
        "\t----%n"
        "\tRDI: %Xs%n"
        "\tRAX: %Xs%n"
        "\tRCX: %Xs%n"
        "\tRDX: %Xs%n"
        "\tRBP: %Xs%n"
        "\t----%n"
        //"\tVector: %u1%n"
        "\tError Code: %Xs%n"
        , val
        , (uint16_t)val->SS
        , (uint16_t)val->DS
        , (uint16_t)val->CS
        , val->RSP
        , val->RIP
        , val->RFLAGS
        , val->RDI
        , val->RAX
        , val->RCX
        , val->RDX
        , val->RBP
        //, (uint8_t)val->Vector
        , val->ErrorCode);
}

TerminalWriteResult PrintToDebugTerminal(IsrStatePartial const * const val)
{
    return PrintToTerminal(DebugTerminal, val);
}

/**********************
    IsrState Struct
**********************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, IsrState const * const val)
{
    return term->WriteFormat("ISR state %Xp:%n"
        "\tStack Segment: %X2%n"
        "\tData Segment: %X2%n"
        "\tCode Segment: %X2%n"
        "\tStack Pointer: %Xp%n"
        "\tInstruction Pointer: %Xp%n"
        "\tFlags: %Xs%n"
        "\t----%n"
        "\tRAX: %Xs%n"
        "\tRBX: %Xs%n"
        "\tRCX: %Xs%n"
        "\tRDX: %Xs%n"
        "\tRBP: %Xs%n"
        "\tRDI: %Xs%n"
        "\tRSI: %Xs%n"
        "\tR8 : %Xs%n"
        "\tR9 : %Xs%n"
        "\tR10: %Xs%n"
        "\tR11: %Xs%n"
        "\tR12: %Xs%n"
        "\tR13: %Xs%n"
        "\tR14: %Xs%n"
        "\tR15: %Xs%n"
        "\t----%n"
        //"\tVector: %u1%n"
        "\tError Code: %Xs%n"
        , val
        , (uint16_t)val->SS
        , (uint16_t)val->DS
        , (uint16_t)val->CS
        , val->RSP
        , val->RIP
        , val->RFLAGS
        , val->RAX
        , val->RBX
        , val->RCX
        , val->RDX
        , val->RBP
        , val->RDI
        , val->RSI
        , val->R8
        , val->R9
        , val->R10
        , val->R11
        , val->R12
        , val->R13
        , val->R14
        , val->R15
        //, (uint8_t)val->Vector
        , val->ErrorCode);
}

TerminalWriteResult PrintToDebugTerminal(IsrState const * const val)
{
    return PrintToTerminal(DebugTerminal, val);
}
