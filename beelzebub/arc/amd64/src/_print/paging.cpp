#include <_print/paging.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Terminals;

/***********************
    Pml1Entry Struct
***********************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, Pml1Entry const val)
{
	char str[58] = "                | |   |   |   |   |   |   |   |   |   |\r\n";
	//str[60] = '\r'; str[61] = '\n'; str[62] = 0;

	//	Address writing.

	uint64_t adr = (uint64_t)val.GetAddress();

	for (size_t i = 0; i < 16; ++i)
	{
		uint8_t nib = (adr >> (i * 4)) & 0xF;

		str[15 - i] = (nib > 9 ? '7' : '0') + nib;
	}

	if (val.GetPresent())
		str[17] = 'P';

	if (val.GetXd())
		{ str[19] = 'E'; str[20] = 'X'; str[21] = 'D'; }

	if (val.GetWritable())
		{ str[23] = 'R'; str[24] = '/'; str[25] = 'W'; }
	else
		str[24] = 'R';

	if (val.GetUserland())
		{ str[27] = 'U'; str[28] = '/'; str[29] = 'S'; }
	else
		str[28] = 'S';

	if (val.GetPwt())
		{ str[31] = 'P'; str[32] = 'W'; str[33] = 'T'; }
	if (val.GetPcd())
		{ str[35] = 'P'; str[36] = 'C'; str[37] = 'D'; }
	if (val.GetAccessed())
		{ str[39] = 'A'; str[40] = 'C'; str[41] = 'C'; }
	if (val.GetGlobal())
		{ str[43] = 'G'; str[44] = 'L'; str[45] = 'B'; }
	if (val.GetDirty())
		{ str[47] = 'D'; str[48] = 'R'; str[49] = 'T'; }
	if (val.GetPat())
		{ str[51] = 'P'; str[52] = 'A'; str[53] = 'T'; }

	return term->Write(str);
}

TerminalWriteResult PrintToDebugTerminal(Pml1Entry const val)
{
    return PrintToTerminal(DebugTerminal, val);
}

/******************
    Pml1 Struct
******************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, Pml1 const & val)
{
	TerminalWriteResult tret;
	
	TERMTRY0(term->WriteLine("IND |PML1:  Address  |T|NXB|R/W|U/S|PWT|PCD|ACC|GLB|DRT|PAT|"), tret);

	uint32_t cnt;

	for (size_t i = 0; i < 512; ++i)
	{
		TERMTRY1(term->WriteHex16((uint16_t)i), tret, cnt);
		TERMTRY1(term->Write("|"), tret, cnt);
		
		TERMTRY1(PrintToTerminal(term, val.Entries[i]), tret, cnt);
	}

	return tret;
}

TerminalWriteResult PrintToDebugTerminal(Pml1 const & val)
{
    return PrintToTerminal(DebugTerminal, val);
}

/***********************
    Pml2Entry Struct
***********************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, Pml2Entry const val)
{
	char str[58] = "                | |   |   |   |   |   |   |   |   |   |\r\n";
	//str[60] = '\r'; str[61] = '\n'; str[62] = 0;

	//	Address writing.

	uint64_t adr = (uint64_t)val.GetPml1Ptr();

	for (size_t i = 0; i < 16; ++i)
	{
		uint8_t nib = (adr >> (i * 4)) & 0xF;

		str[15 - i] = (nib > 9 ? '7' : '0') + nib;
	}

	if (val.GetPresent())
	{
		if (val.GetPageSize())
			str[17] = 'P';
		else
			str[17] = 'T';
	}

	if (val.GetXd())
		{ str[19] = 'E'; str[20] = 'X'; str[21] = 'D'; }

	if (val.GetWritable())
		{ str[23] = 'R'; str[24] = '/'; str[25] = 'W'; }
	else
		str[24] = 'R';

	if (val.GetUserland())
		{ str[27] = 'U'; str[28] = '/'; str[29] = 'S'; }
	else
		str[28] = 'S';

	if (val.GetPwt())
		{ str[31] = 'P'; str[32] = 'W'; str[33] = 'T'; }
	if (val.GetPcd())
		{ str[35] = 'P'; str[36] = 'C'; str[37] = 'D'; }
	if (val.GetAccessed())
		{ str[39] = 'A'; str[40] = 'C'; str[41] = 'C'; }
	if (val.GetGlobal())
		{ str[43] = 'G'; str[44] = 'L'; str[45] = 'B'; }
	if (val.GetDirty())
		{ str[47] = 'D'; str[48] = 'R'; str[49] = 'T'; }
	if (val.GetPat())
		{ str[51] = 'P'; str[52] = 'A'; str[53] = 'T'; }

	return term->Write(str);
}

TerminalWriteResult PrintToDebugTerminal(Pml2Entry const val)
{
    return PrintToTerminal(DebugTerminal, val);
}

/******************
    Pml2 Struct
******************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, Pml2 const & val)
{
	TerminalWriteResult tret;
	
	TERMTRY0(term->WriteLine("IND |PML2:  Address  |T|NXB|R/W|U/S|PWT|PCD|ACC|GLB|DRT|PAT|"), tret);

	uint32_t cnt;

	for (size_t i = 0; i < 512; ++i)
	{
		TERMTRY1(term->WriteHex16((uint16_t)i), tret, cnt);
		TERMTRY1(term->Write("|"), tret, cnt);
		
		TERMTRY1(PrintToTerminal(term, val.Entries[i]), tret, cnt);
	}

	return tret;
}

TerminalWriteResult PrintToDebugTerminal(Pml2 const & val)
{
    return PrintToTerminal(DebugTerminal, val);
}

/***********************
    Pml3Entry Struct
***********************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, Pml3Entry const val)
{
	char str[58] = "                | |   |   |   |   |   |   |   |   |   |\r\n";
	//str[60] = '\r'; str[61] = '\n'; str[62] = 0;

	//	Address writing.

	uint64_t adr = (uint64_t)val.GetPml2Ptr();

	for (size_t i = 0; i < 16; ++i)
	{
		uint8_t nib = (adr >> (i * 4)) & 0xF;

		str[15 - i] = (nib > 9 ? '7' : '0') + nib;
	}

	if (val.GetPresent())
	{
		if (val.GetPageSize())
			str[17] = 'P';
		else
			str[17] = 'T';
	}

	if (val.GetXd())
		{ str[19] = 'E'; str[20] = 'X'; str[21] = 'D'; }

	if (val.GetWritable())
		{ str[23] = 'R'; str[24] = '/'; str[25] = 'W'; }
	else
		str[24] = 'R';

	if (val.GetUserland())
		{ str[27] = 'U'; str[28] = '/'; str[29] = 'S'; }
	else
		str[28] = 'S';

	if (val.GetPwt())
		{ str[31] = 'P'; str[32] = 'W'; str[33] = 'T'; }
	if (val.GetPcd())
		{ str[35] = 'P'; str[36] = 'C'; str[37] = 'D'; }
	if (val.GetAccessed())
		{ str[39] = 'A'; str[40] = 'C'; str[41] = 'C'; }
	if (val.GetGlobal())
		{ str[43] = 'G'; str[44] = 'L'; str[45] = 'B'; }
	if (val.GetDirty())
		{ str[47] = 'D'; str[48] = 'R'; str[49] = 'T'; }
	if (val.GetPat())
		{ str[51] = 'P'; str[52] = 'A'; str[53] = 'T'; }

	return term->Write(str);
}

TerminalWriteResult PrintToDebugTerminal(Pml3Entry const val)
{
    return PrintToTerminal(DebugTerminal, val);
}

/******************
    Pml3 Struct
******************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, Pml3 const & val)
{
	TerminalWriteResult tret;
	
	TERMTRY0(term->WriteLine("IND |PML3:  Address  |T|NXB|R/W|U/S|PWT|PCD|ACC|GLB|DRT|PAT|"), tret);

	uint32_t cnt;

	for (size_t i = 0; i < 512; ++i)
	{
		TERMTRY1(term->WriteHex16((uint16_t)i), tret, cnt);
		TERMTRY1(term->Write("|"), tret, cnt);
		
		TERMTRY1(PrintToTerminal(term, val.Entries[i]), tret, cnt);
	}

	return tret;
}

TerminalWriteResult PrintToDebugTerminal(Pml3 const & val)
{
    return PrintToTerminal(DebugTerminal, val);
}

/***********************
    Pml4Entry Struct
***********************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, Pml4Entry const val)
{
	char str[58] = "                | |   |   |   |   |   |   |\r\n";
	//str[60] = '\r'; str[61] = '\n'; str[62] = 0;

	//	Address writing.

	uint64_t adr = (uint64_t)val.GetPml3Ptr();

	for (size_t i = 0; i < 16; ++i)
	{
		uint8_t nib = (adr >> (i * 4)) & 0xF;

		str[15 - i] = (nib > 9 ? '7' : '0') + nib;
	}

	if (val.GetPresent())
		str[17] = 'T';

	if (val.GetXd())
		{ str[19] = 'E'; str[20] = 'X'; str[21] = 'D'; }

	if (val.GetWritable())
		{ str[23] = 'R'; str[24] = '/'; str[25] = 'W'; }
	else
		str[24] = 'R';

	if (val.GetUserland())
		{ str[27] = 'U'; str[28] = '/'; str[29] = 'S'; }
	else
		str[28] = 'S';

	if (val.GetPwt())
		{ str[31] = 'P'; str[32] = 'W'; str[33] = 'T'; }
	if (val.GetPcd())
		{ str[35] = 'P'; str[36] = 'C'; str[37] = 'D'; }
	if (val.GetAccessed())
		{ str[39] = 'A'; str[40] = 'C'; str[41] = 'C'; }

	return term->Write(str);
}

TerminalWriteResult PrintToDebugTerminal(Pml4Entry const val)
{
    return PrintToTerminal(DebugTerminal, val);
}

/******************
    Pml4 Struct
******************/

TerminalWriteResult PrintToTerminal(TerminalBase * const term, Pml4 const & val)
{
	TerminalWriteResult tret;
	
	TERMTRY0(term->WriteLine("IND |PML4:  Address  |T|NXB|R/W|U/S|PWT|PCD|ACC|"), tret);
	
	uint32_t cnt;

	for (size_t i = 0; i < 512; ++i)
	{
		TERMTRY1(term->WriteHex16((uint16_t)i), tret, cnt);
		TERMTRY1(term->Write("|"), tret, cnt);
		
		TERMTRY1(PrintToTerminal(term, val.Entries[i]), tret, cnt);
	}

	return tret;
}

TerminalWriteResult PrintToDebugTerminal(Pml4 const & val)
{
    return PrintToTerminal(DebugTerminal, val);
}
