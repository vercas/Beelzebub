#include <system/registers_x86_print.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::System;

/**********************
    Ia32Efer Struct
**********************/

TerminalWriteResult Beelzebub::System::PrintToTerminal(TerminalBase * const term, Ia32Efer const val)
{
    char str[32] = "{IA32_EFER|SCE0|LME0|LMA0|NXE0}";

    if (val.GetSyscallEnable())
        str[14] = '1';
    if (val.GetLongModeEnable())
        str[19] = '1';
    if (val.GetLongModeActive())
        str[24] = '1';
    if (val.GetNonExecuteEnable())
        str[29] = '1';

    return term->Write(str);
}
