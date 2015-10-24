#include <_print/isr.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

/**********************
    IsrState Struct
**********************/

TerminalWriteResult Beelzebub::System::PrintToTerminal(TerminalBase * const term, IsrState const * const val)
{
    return term->WriteFormat("ISR state %Xp:%n"
        "\tStack Segment: %X2%n"
        "\tCode Segment: %X2%n"
        "\tStack Pointer: %Xp%n"
        "\tInstruction Pointer: %Xp%n"
        "\tFlags: %Xs%n"
        "\t----%n"
        "\tVector: %u1%n"
        "\tError Code: %Xs%n"
        , val
        , (uint16_t)val->SS
        , (uint16_t)val->CS
        , val->RSP
        , val->RIP
        , val->RFLAGS
        , (uint8_t)val->Vector
        , val->ErrorCode);
}

TerminalWriteResult Beelzebub::System::PrintToDebugTerminal(IsrState const * const val)
{
    return PrintToTerminal(Beelzebub::Debug::DebugTerminal, val);
}
