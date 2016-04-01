/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#pragma once

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

		VbeTerminal() : TerminalBase( nullptr ), VideoMemory((uintptr_t)nullptr) { }
        VbeTerminal(uintptr_t const mem, uint16_t wid, uint16_t hei, uint32_t pit, uint8_t bytesPerPixel);

		/*  Writing  */

        virtual TerminalWriteResult WriteUtf8At(char const * const c, int16_t const x, int16_t const y);

        /*  Positioning  */

        virtual TerminalCoordinates GetSize();

        /*  Remapping  */

        __cold void RemapMemory(uintptr_t newAddr);

	//private:

		uint16_t Width, Height;
		uint32_t Pitch;
		uintptr_t VideoMemory;
        uint8_t BytesPerPixel;
	};
}}
