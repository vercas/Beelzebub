#pragma once

#include <metaprogramming.h>

namespace Beelzebub
{
	class Cpu
	{
	public:
		static const bool CanHalt = true;

		static __bland void Halt();
	};
}
