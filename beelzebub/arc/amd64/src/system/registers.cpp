#include <arc/system/registers.hpp>
#include <terminals/base.hpp>
#include <handles.h>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::System;

/*****************
    Cr0 Struct
*****************/

/*  Debug  */

TerminalWriteResult Cr0::PrintToTerminal(TerminalBase * const term) const
{
    char str[52] = "{CR0|PME0|MC0|Em0|TS0|ET0|NE0|WP0|AM0|NWT0|CD0|Pg0}";

    if (this->GetProtectedModeEnable())
        str[ 8] = '1';
    if (this->GetMonitorCoprocessor())
        str[12] = '1';
    if (this->GetEmulation())
        str[16] = '1';
    if (this->GetTaskSwitched())
        str[20] = '1';
    if (this->GetExtensionType())
        str[24] = '1';
    if (this->GetNumericError())
        str[28] = '1';
    if (this->GetWriteProtect())
        str[32] = '1';
    if (this->GetAlignmentMask())
        str[36] = '1';
    if (this->GetNotWriteThrough())
        str[41] = '1';
    if (this->GetCacheDisable())
        str[45] = '1';
    if (this->GetPaging())
        str[49] = '1';

    return term->Write(str);
}

/*****************
    Cr3 Struct
*****************/

/*  Debug  */

TerminalWriteResult Cr3::PrintToTerminal(TerminalBase * const term) const
{
    char str[37] = "{CR3|                |   |PWT0|PCD0}";

    uint64_t adr = (uint64_t)this->GetAddress();

    for (size_t i = 0; i < 16; ++i)
    {
        uint8_t nib = (adr >> (i * 4)) & 0xF;

        str[20 - i] = (nib > 9 ? '7' : '0') + nib;
    }

    uint64_t pid = (uint64_t)this->GetPcid();

    for (size_t i = 0; i < 3; ++i)
    {
        uint8_t nib = (pid >> (i * 4)) & 0xF;

        str[24 - i] = (nib > 9 ? '7' : '0') + nib;
    }

    if (this->GetPwt())
        str[29] = '1';
    if (this->GetPcd())
        str[34] = '1';

    return term->Write(str);
}

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


