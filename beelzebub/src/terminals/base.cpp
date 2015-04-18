#include <terminals/base.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

/*************************
	TerminalBase class
*************************/

/*  Writing  */

TerminalWriteResult TerminalBase::WriteAt(const char * const str, const TerminalCoordinates pos)
{
	if (!this->Capabilities.CanOutput)
		return {Result::UnsupportedOperation, 0U, InvalidCoordinates};
	//  First of all, the terminal needs to be capable of output.

	TerminalCoordinates size;

	if (this->Capabilities.CanGetBufferSize)
		size = this->GetBufferSize();
	else if (this->Capabilities.CanGetSize)
		size = this->GetSize();
	else
		return {Result::UnsupportedOperation, 0U, InvalidCoordinates};
	//  A buffer or window size is required to prevent drawing outside
	//  the boundaries.

	uint16_t tabWidth = TerminalBase::DefaultTabulatorWidth;

	if (this->Capabilities.CanGetTabulatorWidth)
		tabWidth = this->GetTabulatorWidth();
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

			TerminalWriteResult tmp = this->WriteAt(c, x, y);

			if (tmp.Result != Result::Okay)
				return {tmp.Result, i, {x, y}};

			x++;
		}
	}

	if (this->Capabilities.CanSetOutputPosition)
	{
		Result res = this->SetCurrentPosition(x, y);
	
		return {res, i, {x, y}};
	}

	return {Result::Okay, i, {x, y}};
}

TerminalWriteResult TerminalBase::WriteAt(const char c, const TerminalCoordinates pos)
{
	return this->WriteAt(c, pos.X, pos.Y);
}

TerminalWriteResult TerminalBase::Write(const char c)
{
	if (!(this->Capabilities.CanOutput
	   && this->Capabilities.CanGetOutputPosition))
		return {Result::UnsupportedOperation, 0U, InvalidCoordinates};

	return this->WriteAt(c, this->GetCurrentPosition());
}

TerminalWriteResult TerminalBase::Write(const char * const str)
{
	if (!(this->Capabilities.CanOutput
	   && this->Capabilities.CanGetOutputPosition))
		return {Result::UnsupportedOperation, 0U, InvalidCoordinates};

	return this->WriteAt(str, this->GetCurrentPosition());
}

TerminalWriteResult TerminalBase::WriteLine(const char * const str)
{
	TerminalWriteResult tmp = this->Write(str);

	if (tmp.Result != Result::Okay)
		return tmp;

	return this->Write("\r\n");
}

/*  Positioning  */

Result TerminalBase::SetCursorPosition(const TerminalCoordinates pos)
{
	return this->SetCursorPosition(pos.X, pos.Y);
}

Result TerminalBase::SetCurrentPosition(const int16_t x, const int16_t y)
{
	return this->SetCurrentPosition({x, y});
}

Result TerminalBase::SetCurrentPosition(const TerminalCoordinates pos)
{
	if (!this->Capabilities.CanSetOutputPosition)
		return Result::UnsupportedOperation;

	this->CurrentPosition = pos;

	return Result::Okay;
}

TerminalCoordinates TerminalBase::GetCurrentPosition()
{
	if (!this->Capabilities.CanGetOutputPosition)
		return InvalidCoordinates;

	return this->CurrentPosition;
}

Result TerminalBase::SetSize(const TerminalCoordinates pos)
{
	return this->SetSize(pos.X, pos.Y);
}

Result TerminalBase::SetBufferSize(const TerminalCoordinates pos)
{
	return this->SetBufferSize(pos.X, pos.Y);
}

Result TerminalBase::SetTabulatorWidth(const uint16_t w)
{
	if (!this->Capabilities.CanSetTabulatorWidth)
		return Result::UnsupportedOperation;

	this->TabulatorWidth = w;

	return Result::Okay;
}

uint16_t TerminalBase::GetTabulatorWidth()
{
	if (!this->Capabilities.CanGetTabulatorWidth)
		return ~((uint16_t)0);

	return this->TabulatorWidth;
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
