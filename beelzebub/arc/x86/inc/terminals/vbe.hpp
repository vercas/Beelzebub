#pragma once

#include <metaprogramming.h>
#include <terminals/base.hpp>

#define NOCOL  ((uint32_t)0x90011337)
#define INVCOL ((uint32_t)0x42666616)
//	Immature? Maybe.

#define VBE_BACKGROUND ((uint32_t)0xFF262223)
#define VBE_TEXT       ((uint32_t)0xFFDFE0E6)
#define VBE_SPLASH     ((uint32_t)0xFF121314)
//	A random style of my choosing

namespace Beelzebub { namespace Terminals
{
	class VbeTerminal : public TerminalBase
	{
	public:

		/*  Constructors  */

		__bland VbeTerminal() : TerminalBase( nullptr ), VideoMemory(nullptr) { }
        __bland VbeTerminal(const uintptr_t mem, uint16_t wid, uint16_t hei, uint32_t pit, uint8_t bytesPerPixel);

		/*  Writing  */

        static __bland TerminalWriteResult WriteCharAtXy(TerminalBase * const term, const char c, const int16_t cx, const int16_t cy);

        /*  Positioning  */

        static __bland TerminalCoordinates VbeGetSize(TerminalBase * const term);

        /*  Remapping  */

        __cold __bland void RemapMemory(uintptr_t newAddr);

	//private:

		uint16_t Width, Height;
		uint32_t Pitch;
		uintptr_t VideoMemory;
        uint8_t BytesPerPixel;
	};
}}
