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

/***************************
    GdtTss64Entry Struct
***************************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, GdtTss64Entry const val)
{
    return term->WriteFormat("TSS|%X8:%X4|P%t|A%t|B%t|%n"
        , val.GetBase(), val.GetLimit(), val.GetPresent(), val.GetAvailable()
        , val.GetBusy());
}

TerminalWriteResult PrintToDebugTerminal(GdtTss64Entry const val)
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
        
        GdtEntryShort * entry = &(val.Pointer->Entries[0]) + i;

        if ((  entry->GetSystemDescriptorType() == GdtSystemEntryType::TssAvailable
            || entry->GetSystemDescriptorType() == GdtSystemEntryType::TssBusy     )
            && !entry->GetSystem() && !entry->GetLong() && !entry->GetSize())
        {
            TERMTRY1(PrintToTerminal(term, *reinterpret_cast<GdtTss64Entry *>(entry)), tret, cnt);

            ++i, j += 8;
            //  This is twice as big.
        }
        else
            TERMTRY1(PrintToTerminal(term, *entry), tret, cnt);
    }

    return tret;
}

TerminalWriteResult PrintToDebugTerminal(GdtRegister const val)
{
    return PrintToTerminal(DebugTerminal, val);
}
