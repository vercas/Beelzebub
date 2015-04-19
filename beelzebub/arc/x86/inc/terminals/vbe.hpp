#pragma once

#include <terminals/base.hpp>

namespace Beelzebub { namespace Terminals
{
	class VbeTerminal : public TerminalBase
	{
	public:

		/*  Constructors  */

		__bland VbeTerminal() : TerminalBase( nullptr ) { }

		/*  Writing  */

		// stuff!

	private:

		uint16_t Width, Height;
		uint32_t Pitch;
		uintptr_t VideoMemory;
	};
}}
