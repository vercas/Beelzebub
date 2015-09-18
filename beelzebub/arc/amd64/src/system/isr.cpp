#include <system/isr.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

/**********************
    IsrState Struct
**********************/

/*  Debug  */

TerminalWriteResult IsrState::PrintToTerminal(TerminalBase * const term) const
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
        , this
        , (uint16_t)this->SS
        , (uint16_t)this->CS
        , this->RSP
        , this->RIP
        , this->RFLAGS
        , (uint8_t)this->Vector
        , this->ErrorCode);
}

TerminalWriteResult IsrState::PrintToDebugTerminal() const
{
    return this->PrintToTerminal(Beelzebub::Debug::DebugTerminal);
}
