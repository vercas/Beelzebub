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

#include <beel/terminals/base.hpp>
#include "system/interrupts.hpp"

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

__ENUM_TO_STRING_IMPL(IdtGateType, ENUM_IDTGATETYPE, Beelzebub::System)

static const char * getIdtGateType(IdtGateType const type)
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
    DEBUG PRINTING
*********************/

namespace Beelzebub { namespace Terminals
{
    template<>
    TerminalBase & operator << <IdtGate>(TerminalBase & term, IdtGate const val)
    {
        term.WriteFormat("|%Xp|%X2| %u1 |%s|%b%b|%t|"
                , val.GetOffset(), val.GetSegment(), val.GetIst()
                , getIdtGateType(val.GetType()), val.GetDplHigh(), val.GetDplLow()
                , val.GetPresent());

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<IdtGate>(TerminalBase * term, IdtGate const * val, char const * format)
    {
        if (format == nullptr)
            return term->WriteFormat("|%Xp|%X2| %u1 |%s|%b%b|%t|"
                , val->GetOffset(), val->GetSegment(), val->GetIst()
                , getIdtGateType(val->GetType()), val->GetDplHigh(), val->GetDplLow()
                , val->GetPresent());
        else
            return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
    }

    template<>
    TerminalWriteResult WriteFormattedValue<IdtRegister>(TerminalBase * term, IdtRegister const * val, char const * format)
    {
        TerminalWriteResult tret;
        
        TERMTRY0(term->WriteFormat("IDT Register: %Xp %X2;%n"
            "Ind|     Offset     |Segm|IST|Type|PL|P|%n"
            , val->Pointer, val->Size), tret);

        uint32_t cnt;

        for (size_t i = 0, j = 0; j < val->Size && i < 256; ++i, j += 16)
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
            
            TERMTRY1(WriteFormattedValue<IdtGate>(term, val->Pointer->Entries + i, nullptr), tret, cnt);

            TERMTRY1(term->WriteLine(), tret, cnt);
        }

        return tret;
    }
}}
