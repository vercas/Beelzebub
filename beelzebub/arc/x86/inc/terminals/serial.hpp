#pragma once

#include <terminals/base.hpp>
#include <arc/ports.hpp>

namespace Beelzebub { namespace Terminals
{
	using namespace Ports;

	//TerminalDescriptor SerialTerminalDescriptor;

	class SerialTerminal : public TerminalBase
	{
	public:

		/*  Constructors  */

		__bland SerialTerminal() : TerminalBase( nullptr ), Port(0) { }
		__bland SerialTerminal(const SerialPort port);

		/*  Writing  */

		static __bland TerminalWriteResult WriteChar(TerminalBase * const term, const char c);
		static __bland TerminalWriteResult WriteString(TerminalBase * const term, const char * const str);
		static __bland TerminalWriteResult WriteStringLine(TerminalBase * const term, const char * const str);

	private:

		SerialPort Port;
	};
}}
