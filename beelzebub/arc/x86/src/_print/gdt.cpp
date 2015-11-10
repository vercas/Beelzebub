#include <_print/gdt.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

/***************************
    GdtEntryShort Struct
***************************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, GdtEntryShort const val)
{
    return term->WriteFormat("%X4|%X4|%b%b|%t| %t | %t |%t|%t|%t|%t|%t|%t|%t|%n"
        , val.GetBase(), val.GetLimit(), val.GetDplHigh(), val.GetDplLow()
        , val.GetAccessed(), val.GetRw(), val.GetDc(), val.GetEx()
        , val.GetSystem(), val.GetPresent(), val.GetAvailable(), val.GetLong()
        , val.GetSize(), val.GetGranularity());
}

TerminalWriteResult PrintToDebugTerminal(GdtEntryShort const val)
{
    return PrintToTerminal(DebugTerminal, val);
}

/*************************
    GdtRegister Struct
*************************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, GdtRegister const val)
{
    TerminalWriteResult tret;
    
    TERMTRY0(term->WriteFormat("GDT Register: %Xp %X2;%nAddr|  Base  | Length |PL|A|R/W|D/C|X|S|P|A|L|s|g|%n"
        , val.Pointer, val.Size), tret);

    uint32_t cnt;

    for (size_t i = 0, j = 0; j < val.Size; ++i, j += 8)
    {
        TERMTRY1(term->WriteHex16((uint16_t)i), tret, cnt);
        TERMTRY1(term->Write("|"), tret, cnt);
        
        TERMTRY1(PrintToTerminal(term, val.Pointer->Entries[i]), tret, cnt);
    }

    return tret;
}

TerminalWriteResult PrintToDebugTerminal(GdtRegister const val)
{
    return PrintToTerminal(DebugTerminal, val);
}
