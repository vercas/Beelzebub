#include <terminals/base.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

/*************************
	TerminalBase class
*************************/

/*  STATICS  */

/*  Writing  */

TerminalWriteResult TerminalBase::DefaultWriteCharAtXy(TerminalBase * const term, const char c, const int16_t x, const int16_t y)
{
	return {Result::UnsupportedOperation, 0U, InvalidCoordinates};
}

TerminalWriteResult TerminalBase::DefaultWriteCharAtCoords(TerminalBase * const term, const char c, const TerminalCoordinates pos)
{
	return term->WriteAt(c, pos.X, pos.Y);
}

TerminalWriteResult TerminalBase::DefaultWriteStringAt(TerminalBase * const term, const char * const str, const TerminalCoordinates pos)
{
	if (!term->Descriptor->Capabilities.CanOutput)
		return {Result::UnsupportedOperation, 0U, InvalidCoordinates};
	//  First of all, the terminal needs to be capable of output.

	TerminalCoordinates size;

	if (term->Descriptor->Capabilities.CanGetBufferSize)
		size = term->GetBufferSize();
	else if (term->Descriptor->Capabilities.CanGetSize)
		size = term->GetSize();
	else
		return {Result::UnsupportedOperation, 0U, InvalidCoordinates};
	//  A buffer or window size is required to prevent drawing outside
	//  the boundaries.

	uint16_t tabWidth = TerminalBase::DefaultTabulatorWidth;

	if (term->Descriptor->Capabilities.CanGetTabulatorWidth)
		tabWidth = term->GetTabulatorWidth();
	//  Tabulator width is required for proper handling of \t

	int16_t x = pos.X, y = pos.Y;
	//  These are used for positioning characters.

	uint32_t i = 0;
	//	Declared here so I know how many characters have been written.

	for (; 0 != str[i]; ++i)
	{
		//	Stop at null, naturally.

		char c = str[i];

		if (c == '\r')
			x = 0; //  Carriage return.
		else if (c == '\n')
			++y;   //  Line feed does not return carriage.
		else if (c == '\t')
			x = (x / tabWidth + 1) * tabWidth;
		else if (c == '\b')
		{
			if (x > 0)
				--x;
			//  Once you go \b, you do actually go back.
		}
		else
		{
			if (x == size.X)
			{  x = 0; y++;  }

			TerminalWriteResult tmp = term->WriteAt(c, x, y);

			if (tmp.Result != Result::Okay)
				return {tmp.Result, i, {x, y}};

			x++;
		}
	}

	if (term->Descriptor->Capabilities.CanSetOutputPosition)
	{
		Result res = term->SetCurrentPosition(x, y);
	
		return {res, i, {x, y}};
	}

	return {Result::Okay, i, {x, y}};
}

TerminalWriteResult TerminalBase::DefaultWriteChar(TerminalBase * const term, const char c)
{
	if (!(term->Descriptor->Capabilities.CanOutput
	   && term->Descriptor->Capabilities.CanGetOutputPosition))
		return {Result::UnsupportedOperation, 0U, InvalidCoordinates};

	return term->WriteAt(c, term->GetCurrentPosition());
}

TerminalWriteResult TerminalBase::DefaultWriteString(TerminalBase * const term, const char * const str)
{
	if (!(term->Descriptor->Capabilities.CanOutput
	   && term->Descriptor->Capabilities.CanGetOutputPosition))
		return {Result::UnsupportedOperation, 0U, InvalidCoordinates};

	return term->WriteAt(str, term->GetCurrentPosition());
}

TerminalWriteResult TerminalBase::DefaultWriteStringLine(TerminalBase * const term, const char * const str)
{
	TerminalWriteResult tmp = term->Write(str);

	if (tmp.Result != Result::Okay)
		return tmp;

	return term->Write("\r\n");
}

/*  Positioning  */


Result TerminalBase::DefaultSetCursorPositionXy(TerminalBase * const term, const int16_t x, const int16_t y)
{
	return Result::UnsupportedOperation;
}

Result TerminalBase::DefaultSetCursorPositionCoords(TerminalBase * const term, const TerminalCoordinates pos)
{
	return term->SetCursorPosition(pos.X, pos.Y);
}

TerminalCoordinates TerminalBase::DefaultGetCursorPosition(TerminalBase * const term)
{
	return InvalidCoordinates;
}


Result TerminalBase::DefaultSetCurrentPositionXy(TerminalBase * const term, const int16_t x, const int16_t y)
{
	return term->SetCurrentPosition({x, y});
}

Result TerminalBase::DefaultSetCurrentPositionCoords(TerminalBase * const term, const TerminalCoordinates pos)
{
	if (!term->Descriptor->Capabilities.CanSetOutputPosition)
		return Result::UnsupportedOperation;

	term->CurrentPosition = pos;

	return Result::Okay;
}

TerminalCoordinates TerminalBase::DefaultGetCurrentPosition(TerminalBase * const term)
{
	if (!term->Descriptor->Capabilities.CanGetOutputPosition)
		return InvalidCoordinates;

	return term->CurrentPosition;
}


Result TerminalBase::DefaultSetSizeXy(TerminalBase * const term, const int16_t w, const int16_t h)
{
	return Result::UnsupportedOperation;
}

Result TerminalBase::DefaultSetSizeCoords(TerminalBase * const term, const TerminalCoordinates pos)
{
	return term->SetSize(pos.X, pos.Y);
}

TerminalCoordinates TerminalBase::DefaultGetSize(TerminalBase * const term)
{
	return InvalidCoordinates;
}


Result TerminalBase::DefaultSetBufferSizeXy(TerminalBase * const term, const int16_t w, const int16_t h)
{
	return Result::UnsupportedOperation;
}

Result TerminalBase::DefaultSetBufferSizeCoords(TerminalBase * const term, const TerminalCoordinates pos)
{
	return term->SetBufferSize(pos.X, pos.Y);
}

TerminalCoordinates TerminalBase::DefaultGetBufferSize(TerminalBase * const term)
{
	return InvalidCoordinates;
}


Result TerminalBase::DefaultSetTabulatorWidth(TerminalBase * const term, const uint16_t w)
{
	if (!term->Descriptor->Capabilities.CanSetTabulatorWidth)
		return Result::UnsupportedOperation;

	term->TabulatorWidth = w;

	return Result::Okay;
}

uint16_t TerminalBase::DefaultGetTabulatorWidth(TerminalBase * const term)
{
	if (!term->Descriptor->Capabilities.CanGetTabulatorWidth)
		return ~((uint16_t)0);

	return term->TabulatorWidth;
}

/*  DYNAMICS  */

/*  Constructor  */

TerminalBase::TerminalBase(const TerminalDescriptor * const desc)
	: Descriptor(desc)
{

}

/*  Writing  */

TerminalWriteResult TerminalBase::WriteAt(const char c, const int16_t x, const int16_t y)
{
	return this->Descriptor->WriteCharAtXy(this, c, x, y);
}

TerminalWriteResult TerminalBase::WriteAt(const char c, const TerminalCoordinates pos)
{
	return this->Descriptor->WriteCharAtCoords(this, c, pos);
}

TerminalWriteResult TerminalBase::WriteAt(const char * const str, const TerminalCoordinates pos)
{
	return this->Descriptor->WriteStringAtCoords(this, str, pos);
}

TerminalWriteResult TerminalBase::Write(const char c)
{
	return this->Descriptor->WriteChar(this, c);
}

TerminalWriteResult TerminalBase::Write(const char * const str)
{
	return this->Descriptor->WriteString(this, str);
}

TerminalWriteResult TerminalBase::WriteLine(const char * const str)
{
	return this->Descriptor->WriteLineString(this, str);
}

/*  Positioning  */

#define I_HATE_REPEATING_MYSELF_1(prop) \
Result TerminalBase::MCATS2(Set, prop)(const int16_t x, const int16_t y) \
{ \
	return this->Descriptor->MCATS3(Set, prop, Xy)(this, x, y); \
} \
Result TerminalBase::MCATS2(Set, prop)(const TerminalCoordinates pos) \
{ \
	return this->Descriptor->MCATS3(Set, prop, Coords)(this, pos); \
} \
TerminalCoordinates TerminalBase::MCATS2(Get, prop)() \
{ \
	return this->Descriptor->MCATS2(Get, prop)(this); \
} \

I_HATE_REPEATING_MYSELF_1(CursorPosition)
I_HATE_REPEATING_MYSELF_1(CurrentPosition)
I_HATE_REPEATING_MYSELF_1(Size)
I_HATE_REPEATING_MYSELF_1(BufferSize)


Result TerminalBase::SetTabulatorWidth(const uint16_t w)
{
	return this->Descriptor->SetTabulatorWidth(this, w);
}

uint16_t TerminalBase::GetTabulatorWidth()
{
	return this->Descriptor->GetTabulatorWidth(this);
}

/*********************************
	TerminalCoordinates struct
*********************************/

inline __bland TerminalCoordinates TerminalCoordinates::operator+(const TerminalCoordinates other)
{
	return { (int16_t)(this->X + other.X), (int16_t)(this->Y + other.Y) };
}

inline __bland TerminalCoordinates TerminalCoordinates::operator-(const TerminalCoordinates other)
{
	return { (int16_t)(this->X - other.X), (int16_t)(this->Y - other.Y) };
}
