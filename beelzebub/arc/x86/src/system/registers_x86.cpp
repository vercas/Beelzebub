#include <system/registers_x86.hpp>
#include <terminals/base.hpp>
#include <handles.h>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::System;

/**********************
    Ia32Efer Struct
**********************/

/*  Debug  */

TerminalWriteResult Ia32Efer::PrintToTerminal(TerminalBase * const term) const
{
    char str[32] = "{IA32_EFER|SCE0|LME0|LMA0|NXE0}";

    if (this->GetSyscallEnable())
        str[14] = '1';
    if (this->GetLongModeEnable())
        str[19] = '1';
    if (this->GetLongModeActive())
        str[24] = '1';
    if (this->GetNonExecuteEnable())
        str[29] = '1';

    return term->Write(str);
}
