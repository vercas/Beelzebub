#include <arc/terminals/serial.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Ports;
using namespace Beelzebub::Terminals;

/****************************
	SerialTerminal struct
*****************************/

/*	Constructors	*/

SerialTerminal::SerialTerminal(const SerialPort port)
	: Port(port)
{
	this->Capabilities.CanOutput = 
	this->Capabilities.SequentialOutput = true;
}

/*  Writing  */

TerminalWriteResult SerialTerminal::WriteAt(const char c, const int16_t x, const int16_t y)
{
	return {Result::UnsupportedOperation, 0U, InvalidCoordinates};
}

TerminalWriteResult SerialTerminal::Write(const char c)
{
	this->Port.Write(c, true);

	return {Result::Okay, 1U, InvalidCoordinates};
}

TerminalWriteResult SerialTerminal::Write(const char * const str)
{
	return {Result::Okay, (uint32_t)this->Port.WriteNtString(str), InvalidCoordinates};
}

TerminalWriteResult SerialTerminal::WriteLine(const char * const str)
{
	size_t n = this->Port.WriteNtString(str);
	n += this->Port.WriteNtString("\r\n");

	return {Result::Okay, (uint32_t)n, InvalidCoordinates};
}

/*  Positioning  */

Result SerialTerminal::SetCursorPosition(const int16_t x, const int16_t y)
{
	return Result::UnsupportedOperation;
}

TerminalCoordinates SerialTerminal::GetCursorPosition()
{
	return InvalidCoordinates;
}

Result SerialTerminal::SetSize(const int16_t w, const int16_t h)
{
	return Result::UnsupportedOperation;
}

TerminalCoordinates SerialTerminal::GetSize()
{
	return InvalidCoordinates;
}

Result SerialTerminal::SetBufferSize(const int16_t w, const int16_t h)
{
	return Result::UnsupportedOperation;
}

TerminalCoordinates SerialTerminal::GetBufferSize()
{
	return InvalidCoordinates;
}
