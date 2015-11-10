#include <_print/idt.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

static __bland const char * getIdtGateType(IdtGateType const type)
{
    switch (type)
    {
    case IdtGateType::Unused00:
        return "Un00";
    case IdtGateType::Unused01:
        return "Un01";
    case IdtGateType::Unused02:
        return "Un02";
    case IdtGateType::Unused03:
        return "Un03";
    case IdtGateType::Unused04:
        return "Un04";
    case IdtGateType::Unused08:
        return "Un08";
    case IdtGateType::Unused09:
        return "Un09";
    case IdtGateType::Unused10:
        return "Un10";
    case IdtGateType::Unused11:
        return "Un11";
    case IdtGateType::Unused12:
        return "Un12";
    case IdtGateType::Unused13:
        return "Un13";

    case IdtGateType::TaskGate:
        return "Task";

    case IdtGateType::InterruptGate16:
        return "Ig16";
    case IdtGateType::InterruptGate:
        return "IntG";

    case IdtGateType::TrapGate16:
        return "Tg16";
    case IdtGateType::TrapGate:
        return "Trap";

    default:
        return "UNKN";
    }
}

/*********************
    IdtGate Struct
*********************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, IdtGate const val)
{
    return term->WriteFormat("%Xp|%X2| %u1 |%s|%b%b|%t|%n"
        , val.GetOffset(), val.GetSegment(), val.GetIst()
        , getIdtGateType(val.GetType()), val.GetDplHigh(), val.GetDplLow()
        , val.GetPresent());
}

TerminalWriteResult PrintToDebugTerminal(IdtGate const val)
{
    return PrintToTerminal(DebugTerminal, val);
}

/*************************
    IdtRegister Struct
*************************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, IdtRegister const val)
{
    TerminalWriteResult tret;
    
    TERMTRY0(term->WriteFormat("IDT Register: %Xp %X2;%n"
        "Ind|     Offset     |Segm|IST|Type|PL|P|%n"
        , val.Pointer, val.Size), tret);

    uint32_t cnt;

    for (size_t i = 0, j = 0; j < val.Size && i < 256; ++i, j += 16)
    {
        if (i < 100)
        {
            TERMTRY1(term->Write(' '), tret, cnt);

            if (i < 10)
            {
                TERMTRY1(term->Write(' '), tret, cnt);
            }
        }

        TERMTRY1(term->WriteUIntD((uint64_t)i), tret, cnt);
        TERMTRY1(term->Write('|'), tret, cnt);
        
        TERMTRY1(PrintToTerminal(term, val.Pointer->Entries[i]), tret, cnt);
    }

    return tret;
}

TerminalWriteResult PrintToDebugTerminal(IdtRegister const val)
{
    return PrintToTerminal(DebugTerminal, val);
}
