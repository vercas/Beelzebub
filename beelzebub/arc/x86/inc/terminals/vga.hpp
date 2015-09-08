#pragma once

#include <metaprogramming.h>
#include <terminals/base.hpp>
#include <terminals/colors.hpp>

namespace Beelzebub { namespace Terminals { namespace Vga
{
	using namespace Colors;

	void * const BufferPtr = (void *)0xB8000;
	uint8_t const BufferWidth = 80;
    uint8_t const BufferHeight = 25;

    uint8_t const DefaultColor = 0x07;

	struct Cell
	{
		char Character;
        uint8_t Colors;

		/*	Constructors	*/
		Cell() __bland;
		Cell(char const c, Color16 const fg, Color16 const bg) __bland;

		/*	Colors	*/
		Color16 GetBackgroundColor() const __bland;
		Color16 SetBackgroundColor(Color16 const val) __bland;
		Color16 GetForegroundColor() const __bland;
		Color16 SetForegroundColor(Color16 const val) __bland;
	} __packed;
}}}
