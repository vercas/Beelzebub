#pragma once

#include <metaprogramming.h>
#include <terminals/colors.hpp>

namespace Beelzebub { namespace Terminals { namespace Vga {
	using namespace Colors;

	void * const BufferPtr = (void *)0xB8000;
	u8 const BufferWidth = 80;
	u8 const BufferHeight = 25;

	u8 const DefaultColor = 0x07;

	struct Cell
	{
		char Character;
		u8 Colors;

		/*	Constructors	*/
		Cell() blandfunc;
		Cell(char const c, Color16 const fg, Color16 const bg) blandfunc;

		/*	Colors	*/
		Color16 GetBackgroundColor() const blandfunc;
		Color16 SetBackgroundColor(Color16 const val) blandfunc;
		Color16 GetForegroundColor() const blandfunc;
		Color16 SetForegroundColor(Color16 const val) blandfunc;
	} __attribute__((packed));
}}}
