#include <memory/paging.hpp>
#include <terminals/base.hpp>
#include <handles.h>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Memory::Paging;
using namespace Beelzebub::Terminals;

/***********************
    Pml1Entry Struct
***********************/

/*  Debug  */

TerminalWriteResult Pml1Entry::PrintToTerminal(TerminalBase * const term) const
{
	char str[58] = "                | |   |   |   |   |   |   |   |   |   |\r\n";
	//str[60] = '\r'; str[61] = '\n'; str[62] = 0;

	//	Address writing.

	uint64_t adr = (uint64_t)this->GetAddress();

	for (size_t i = 0; i < 16; ++i)
	{
		uint8_t nib = (adr >> (i * 4)) & 0xF;

		str[15 - i] = (nib > 9 ? '7' : '0') + nib;
	}

	if (this->GetPresent())
		str[17] = 'P';

	if (this->GetXd())
		{ str[19] = 'E'; str[20] = 'X'; str[21] = 'D'; }

	if (this->GetWritable())
		{ str[23] = 'R'; str[24] = '/'; str[25] = 'W'; }
	else
		str[24] = 'R';

	if (this->GetUserland())
		{ str[27] = 'U'; str[28] = '/'; str[29] = 'S'; }
	else
		str[28] = 'S';

	if (this->GetPwt())
		{ str[31] = 'P'; str[32] = 'W'; str[33] = 'T'; }
	if (this->GetPcd())
		{ str[35] = 'P'; str[36] = 'C'; str[37] = 'D'; }
	if (this->GetAccessed())
		{ str[39] = 'A'; str[40] = 'C'; str[41] = 'C'; }
	if (this->GetGlobal())
		{ str[43] = 'G'; str[44] = 'L'; str[45] = 'B'; }
	if (this->GetDirty())
		{ str[47] = 'D'; str[48] = 'R'; str[49] = 'T'; }
	if (this->GetPat())
		{ str[51] = 'P'; str[52] = 'A'; str[53] = 'T'; }

	return term->Write(str);
}

/******************
    Pml1 Struct
******************/

/*  Debug  */

TerminalWriteResult Pml1::PrintToTerminal(TerminalBase * const term) const
{
	TerminalWriteResult tret;
	
	TERMTRY0(term->WriteLine("IND |PML1:  Address  |T|NXB|R/W|U/S|PWT|PCD|ACC|GLB|DRT|PAT|"), tret);

	uint32_t cnt;

	for (size_t i = 0; i < 512; ++i)
	{
		TERMTRY1(term->WriteHex16((uint16_t)i), tret, cnt);
		TERMTRY1(term->Write("|"), tret, cnt);
		
		TERMTRY1(this->Entries[i].PrintToTerminal(term), tret, cnt);
	}

	return tret;
}

/***********************
    Pml2Entry Struct
***********************/

/*  Debug  */

TerminalWriteResult Pml2Entry::PrintToTerminal(TerminalBase * const term) const
{
	char str[58] = "                | |   |   |   |   |   |   |   |   |   |\r\n";
	//str[60] = '\r'; str[61] = '\n'; str[62] = 0;

	//	Address writing.

	uint64_t adr = (uint64_t)this->GetPml1Ptr();

	for (size_t i = 0; i < 16; ++i)
	{
		uint8_t nib = (adr >> (i * 4)) & 0xF;

		str[15 - i] = (nib > 9 ? '7' : '0') + nib;
	}

	if (this->GetPresent())
	{
		if (this->GetPageSize())
			str[17] = 'P';
		else
			str[17] = 'T';
	}

	if (this->GetXd())
		{ str[19] = 'E'; str[20] = 'X'; str[21] = 'D'; }

	if (this->GetWritable())
		{ str[23] = 'R'; str[24] = '/'; str[25] = 'W'; }
	else
		str[24] = 'R';

	if (this->GetUserland())
		{ str[27] = 'U'; str[28] = '/'; str[29] = 'S'; }
	else
		str[28] = 'S';

	if (this->GetPwt())
		{ str[31] = 'P'; str[32] = 'W'; str[33] = 'T'; }
	if (this->GetPcd())
		{ str[35] = 'P'; str[36] = 'C'; str[37] = 'D'; }
	if (this->GetAccessed())
		{ str[39] = 'A'; str[40] = 'C'; str[41] = 'C'; }
	if (this->GetGlobal())
		{ str[43] = 'G'; str[44] = 'L'; str[45] = 'B'; }
	if (this->GetDirty())
		{ str[47] = 'D'; str[48] = 'R'; str[49] = 'T'; }
	if (this->GetPat())
		{ str[51] = 'P'; str[52] = 'A'; str[53] = 'T'; }

	return term->Write(str);
}

/******************
    Pml2 Struct
******************/

/*  Debug  */

TerminalWriteResult Pml2::PrintToTerminal(TerminalBase * const term) const
{
	TerminalWriteResult tret;
	
	TERMTRY0(term->WriteLine("IND |PML2:  Address  |T|NXB|R/W|U/S|PWT|PCD|ACC|GLB|DRT|PAT|"), tret);

	uint32_t cnt;

	for (size_t i = 0; i < 512; ++i)
	{
		TERMTRY1(term->WriteHex16((uint16_t)i), tret, cnt);
		TERMTRY1(term->Write("|"), tret, cnt);
		
		TERMTRY1(this->Entries[i].PrintToTerminal(term), tret, cnt);
	}

	return tret;
}

/***********************
    Pml3Entry Struct
***********************/

/*  Debug  */

TerminalWriteResult Pml3Entry::PrintToTerminal(TerminalBase * const term) const
{
	char str[58] = "                | |   |   |   |   |   |   |   |   |   |\r\n";
	//str[60] = '\r'; str[61] = '\n'; str[62] = 0;

	//	Address writing.

	uint64_t adr = (uint64_t)this->GetPml2Ptr();

	for (size_t i = 0; i < 16; ++i)
	{
		uint8_t nib = (adr >> (i * 4)) & 0xF;

		str[15 - i] = (nib > 9 ? '7' : '0') + nib;
	}

	if (this->GetPresent())
	{
		if (this->GetPageSize())
			str[17] = 'P';
		else
			str[17] = 'T';
	}

	if (this->GetXd())
		{ str[19] = 'E'; str[20] = 'X'; str[21] = 'D'; }

	if (this->GetWritable())
		{ str[23] = 'R'; str[24] = '/'; str[25] = 'W'; }
	else
		str[24] = 'R';

	if (this->GetUserland())
		{ str[27] = 'U'; str[28] = '/'; str[29] = 'S'; }
	else
		str[28] = 'S';

	if (this->GetPwt())
		{ str[31] = 'P'; str[32] = 'W'; str[33] = 'T'; }
	if (this->GetPcd())
		{ str[35] = 'P'; str[36] = 'C'; str[37] = 'D'; }
	if (this->GetAccessed())
		{ str[39] = 'A'; str[40] = 'C'; str[41] = 'C'; }
	if (this->GetGlobal())
		{ str[43] = 'G'; str[44] = 'L'; str[45] = 'B'; }
	if (this->GetDirty())
		{ str[47] = 'D'; str[48] = 'R'; str[49] = 'T'; }
	if (this->GetPat())
		{ str[51] = 'P'; str[52] = 'A'; str[53] = 'T'; }

	return term->Write(str);
}

/******************
    Pml3 Struct
******************/

/*  Debug  */

TerminalWriteResult Pml3::PrintToTerminal(TerminalBase * const term) const
{
	TerminalWriteResult tret;
	
	TERMTRY0(term->WriteLine("IND |PML3:  Address  |T|NXB|R/W|U/S|PWT|PCD|ACC|GLB|DRT|PAT|"), tret);

	uint32_t cnt;

	for (size_t i = 0; i < 512; ++i)
	{
		TERMTRY1(term->WriteHex16((uint16_t)i), tret, cnt);
		TERMTRY1(term->Write("|"), tret, cnt);
		
		TERMTRY1(this->Entries[i].PrintToTerminal(term), tret, cnt);
	}

	return tret;
}

/***********************
    Pml4Entry Struct
***********************/

/*  Debug  */

TerminalWriteResult Pml4Entry::PrintToTerminal(TerminalBase * const term) const
{
	char str[58] = "                | |   |   |   |   |   |   |\r\n";
	//str[60] = '\r'; str[61] = '\n'; str[62] = 0;

	//	Address writing.

	uint64_t adr = (uint64_t)this->GetPml3Ptr();

	for (size_t i = 0; i < 16; ++i)
	{
		uint8_t nib = (adr >> (i * 4)) & 0xF;

		str[15 - i] = (nib > 9 ? '7' : '0') + nib;
	}

	if (this->GetPresent())
		str[17] = 'T';

	if (this->GetXd())
		{ str[19] = 'E'; str[20] = 'X'; str[21] = 'D'; }

	if (this->GetWritable())
		{ str[23] = 'R'; str[24] = '/'; str[25] = 'W'; }
	else
		str[24] = 'R';

	if (this->GetUserland())
		{ str[27] = 'U'; str[28] = '/'; str[29] = 'S'; }
	else
		str[28] = 'S';

	if (this->GetPwt())
		{ str[31] = 'P'; str[32] = 'W'; str[33] = 'T'; }
	if (this->GetPcd())
		{ str[35] = 'P'; str[36] = 'C'; str[37] = 'D'; }
	if (this->GetAccessed())
		{ str[39] = 'A'; str[40] = 'C'; str[41] = 'C'; }

	return term->Write(str);
}

/******************
    Pml4 Struct
******************/

/*  Debug  */

TerminalWriteResult Pml4::PrintToTerminal(TerminalBase * const term) const
{
	TerminalWriteResult tret;
	
	TERMTRY0(term->WriteLine("IND |PML4:  Address  |T|NXB|R/W|U/S|PWT|PCD|ACC|"), tret);
	
	uint32_t cnt;

	for (size_t i = 0; i < 512; ++i)
	{
		TERMTRY1(term->WriteHex16((uint16_t)i), tret, cnt);
		TERMTRY1(term->Write("|"), tret, cnt);
		
		TERMTRY1(this->Entries[i].PrintToTerminal(term), tret, cnt);
	}

	return tret;
}
