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
	char str[53] = "CR0|PME0|MC0|Em0|TS0|ET0|NE0|WP0|AM0|NWT0|CD0|Pg0|\r\n";

	if (this->GetProtectedModeEnable())
		str[7] = '1';
	if (this->GetMonitorCoprocessor())
		str[11] = '1';
	if (this->GetEmulation())
		str[15] = '1';
	if (this->GetTaskSwitched())
		str[19] = '1';
	if (this->GetExtensionType())
		str[23] = '1';
	if (this->GetNumericError())
		str[27] = '1';
	if (this->GetWriteProtect())
		str[31] = '1';
	if (this->GetAlignmentMask())
		str[35] = '1';
	if (this->GetNotWriteThrough())
		str[40] = '1';
	if (this->GetCacheDisable())
		str[44] = '1';
	if (this->GetPaging())
		str[48] = '1';

	return term->Write(str);
}

/*****************
    Cr3 Struct
*****************/

/*  Debug  */

TerminalWriteResult Cr3::PrintToTerminal(TerminalBase * const term) const
{
	char str[38] = "CR3|                |   |PWT0|PCD0|\r\n";

	uint64_t adr = (uint64_t)this->GetPml4Ptr();

	for (size_t i = 0; i < 16; ++i)
	{
		uint8_t nib = (adr >> (i * 4)) & 0xF;

		str[19 - i] = (nib > 9 ? '7' : '0') + nib;
	}

	uint64_t pid = (uint64_t)this->GetPcid();

	for (size_t i = 0; i < 3; ++i)
	{
		uint8_t nib = (pid >> (i * 4)) & 0xF;

		str[23 - i] = (nib > 9 ? '7' : '0') + nib;
	}

	if (this->GetPwt())
		str[28] = '1';
	if (this->GetPcd())
		str[33] = '1';

	return term->Write(str);
}


