#pragma once

#include <metaprogramming.h>

namespace Beelzebub { namespace Terminals { namespace Colors {
	///	Possible colors for 16-color terminals.
	enum Color16 : u8
	{
		Black = 0,
		Blue = 1,
		Green = 2,
		Cyan = 3,
		Red = 4,
		Magenta = 5,
		Brown = 6,
		LightGray = 7,
		DarkGray = 8,
		LightBlue = 9,
		LightGreen = 10,
		LightCyan = 11,
		LightRed = 12,
		LightMagenta = 13,
		LightBrown = 14,
		White = 15,
	};
}}}
