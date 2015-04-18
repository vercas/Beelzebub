#pragma once

#include <terminals/base.hpp>
#include <arc/ports.hpp>

namespace Beelzebub { namespace Terminals
{
	using namespace Ports;

	class SerialTerminal : public TerminalBase
	{
	public:

		/*  Cosntructors  */

		__bland SerialTerminal() : Port(0) { }
		__bland SerialTerminal(const SerialPort port);

		/*  Writing  */

		virtual __bland TerminalWriteResult WriteAt(const char c, const int16_t x, const int16_t y) override;

		virtual __bland TerminalWriteResult Write(const char c) override;
		virtual __bland TerminalWriteResult Write(const char * const str) override;
		virtual __bland TerminalWriteResult WriteLine(const char * const str) override;

		/*  Positioning  */

		virtual __bland Result SetCursorPosition(const int16_t x, const int16_t y) override;
		virtual __bland TerminalCoordinates GetCursorPosition() override;

		virtual __bland Result SetSize(const int16_t w, const int16_t h) override;
		virtual __bland TerminalCoordinates GetSize() override;

		virtual __bland Result SetBufferSize(const int16_t w, const int16_t h) override;
		virtual __bland TerminalCoordinates GetBufferSize() override;

	private:

		SerialPort Port;
	};
}}
