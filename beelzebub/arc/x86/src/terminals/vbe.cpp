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

#include <terminals/vbe.hpp>
#include <terminals/font.hpp>
#include <terminals/splash.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

/*  VBE terminal descriptor  */

TerminalCapabilities VbeTerminalCapabilities = {
    true,   //  bool CanOutput;            //  Characters can be written to the terminal.
    false,  //  bool CanInput;             //  Characters can be received from the terminal.
    false,  //  bool CanRead;              //  Characters can be read back from the terminal's output.

    true,   //  bool CanGetOutputPosition; //  Position of next output character can be retrieved.
    true,   //  bool CanSetOutputPosition; //  Position of output characters can be set arbitrarily.
    false,  //  bool CanPositionCursor;    //  Terminal features a positionable cursor.

    true,   //  bool CanGetSize;           //  Terminal (window) size can be retrieved.
    false,  //  bool CanSetSize;           //  Terminal (window) size can be changed.

    false,  //  bool Buffered;             //  Terminal acts as a window over a buffer.
    false,  //  bool CanGetBufferSize;     //  Buffer size can be retrieved.
    false,  //  bool CanSetBufferSize;     //  Buffer size can be changed.
    false,  //  bool CanPositionWindow;    //  The "window" can be positioned arbitrarily over the buffer.

    false,  //  bool CanColorBackground;   //  Area behind/around output characters can be colored.
    false,  //  bool CanColorForeground;   //  Output characters can be colored.
    false,  //  bool FullColor;            //  32-bit BGRA, or ARGB in little endian.
    false,  //  bool ForegroundAlpha;      //  Alpha channel of foreground color is supported. (ignored if false)
    false,  //  bool BackgroundAlpha;      //  Alpha channel of background color is supported. (ignored if false)

    false,  //  bool CanBold;              //  Output characters can be made bold.
    false,  //  bool CanUnderline;         //  Output characters can be underlined.
    false,  //  bool CanBlink;             //  Output characters can blink.

    false,  //  bool CanGetStyle;          //  Current style settings can be retrieved.

    true,   //  bool CanGetTabulatorWidth; //  Tabulator width may be retrieved.
    true,   //  bool CanSetTabulatorWidth; //  Tabulator width may be changed.

    true,   //  bool SequentialOutput;     //  Character sequences can be output without explicit position.

    false,  //  bool SupportsTitle;        //  Supports assignment of a title.

    TerminalType::PixelMatrix    //  TerminalType Type;         //  The known type of the terminal.
};

/*************************
    VbeTerminal struct
*************************/

/*  Constructors    */

VbeTerminal::VbeTerminal(const uintptr_t mem, uint16_t wid, uint16_t hei, uint32_t pit, uint8_t bytesPerPixel)
    : TerminalBase(&VbeTerminalCapabilities)
    , Width(wid)
    , Height(hei)
    , PreSplashWidth(wid - SplashImageWidth)
    , PreSplashHeight(hei - SplashImageHeight)
    , Pitch(pit)
    , VideoMemory(mem)
    , BytesPerPixel(bytesPerPixel)
    , Size({(int16_t)((int32_t)wid / (int32_t)FontWidth), (int16_t)((int32_t)hei / (int32_t)FontHeight)})
{
    for (int x = 0; x < this->Size.X; ++x)
        for (int y = 0; y < this->Size.Y; ++y)
            this->WriteUtf8At(" ", x, y);
}

/*  Writing  */

TerminalWriteResult VbeTerminal::WriteUtf8At(char const * c, const int16_t cx, const int16_t cy)
{
    auto drawBackgroundPixel = [this](size_t x, size_t y, uint32_t * pixel, uint32_t colb, uint32_t cols)
    {
        if (x >= this->PreSplashWidth
         && y >= this->PreSplashHeight)
        {
            size_t const byteInd = (x - this->PreSplashWidth) / 8,
                          bitInd = (x - this->PreSplashWidth) & 7;

            if (SplashImage[(y - this->PreSplashHeight) * (SplashImageWidth / 8) + byteInd] & (0x80 >> bitInd))
            {
                *pixel = cols;

                return;
            }
        }

        if (colb != NOCOL)
            *pixel = colb;
    };

    /* TEMP */ uint32_t const colf = 0xFFDFE0E6;
    /* TEMP */ uint32_t const colb = 0xFF262223;
    /* TEMP */ uint32_t const cols = 0xFF121314;

    size_t const w = FontWidth;
    size_t const h = FontHeight;
    size_t const x = cx * w;
    size_t const y = cy * h;

    size_t line = this->VideoMemory + y * this->Pitch + x * this->BytesPerPixel;
    size_t const byteWidth = w / 8;

    uint32_t i = 1U;   //  Number of bytes in this character.

    if (*c == ' ')
        for (size_t ly = 0; ly < h; ++ly, line += this->Pitch)
            for (size_t lx = 0, col = 0; lx < w; ++lx, col += this->BytesPerPixel)
                drawBackgroundPixel(x + lx, y + ly, (uint32_t *)(line + col), colb, cols);
    else
    {
        uint8_t const * bmp = Font[*c - FontMin];

        if (*reinterpret_cast<unsigned char const *>(c) >= 0x80)
        {
            //  This belongs to a multibyte character... Uh oh...

            goto skip_bitmap;
        }

        for (size_t ly = 0; ly < h; ++ly, line += this->Pitch)
        {
            size_t ind = ly * w / 8;

            for (size_t lx = 0, col = 0; lx < byteWidth; ++lx)
                for (size_t bit = 0; bit < 8; ++bit, col += this->BytesPerPixel)
                    if (bmp[ind + lx] & (0x80 >> bit))
                    {
                        if (colf == INVCOL)
                            *((uint32_t *)(line + col)) ^= 0xFFFFFFFFU;
                        else if (colf != NOCOL)
                            *((uint32_t *)(line + col)) = colf;
                    }
                    else
                        drawBackgroundPixel(x + lx * 8 + bit, y + ly, (uint32_t *)(line + col), colb, cols);
        }
    }

skip_bitmap:
    return {HandleResult::Okay, i, {cx, cy}};
}

Handle VbeTerminal::Flush()
{
    return HandleResult::Okay;
}

/*  Positioning  */

TerminalCoordinates VbeTerminal::GetSize()
{
    return this->Size;
}

/*  Remapping  */

void VbeTerminal::RemapMemory(uintptr_t newAddr)
{
    this->VideoMemory = newAddr;
}
