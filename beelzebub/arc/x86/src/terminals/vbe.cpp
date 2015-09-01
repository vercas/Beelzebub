#include <metaprogramming.h>
#include <terminals/vbe.hpp>
#include <terminals/font.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

/*  VBE terminal descriptor  */

TerminalDescriptor VbeTerminalDescriptor = {
    &VbeTerminal::WriteCharAtXy,
    &TerminalBase::DefaultWriteCharAtCoords,
    &TerminalBase::DefaultWriteStringAt,
    &TerminalBase::DefaultWriteChar,
    &TerminalBase::DefaultWriteString,
    &TerminalBase::DefaultWriteStringVarargs,
    &TerminalBase::DefaultWriteStringLine,

    &TerminalBase::DefaultSetCursorPositionXy,
    &TerminalBase::DefaultSetCursorPositionCoords,
    &TerminalBase::DefaultGetCursorPosition,

    &TerminalBase::DefaultSetCurrentPositionXy,
    &TerminalBase::DefaultSetCurrentPositionCoords,
    &TerminalBase::DefaultGetCurrentPosition,

    &TerminalBase::DefaultSetSizeXy,
    &TerminalBase::DefaultSetSizeCoords,
    &VbeTerminal::VbeGetSize,

    &TerminalBase::DefaultSetBufferSizeXy,
    &TerminalBase::DefaultSetBufferSizeCoords,
    &TerminalBase::DefaultGetBufferSize,

    &TerminalBase::DefaultSetTabulatorWidth,
    &TerminalBase::DefaultGetTabulatorWidth,

    {
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
    }
};

/*************************
    VbeTerminal struct
*************************/

/*  Constructors    */

VbeTerminal::VbeTerminal(const uintptr_t mem, uint16_t wid, uint16_t hei, uint32_t pit, uint8_t bytesPerPixel)
    : TerminalBase(&VbeTerminalDescriptor)
    , Width(wid)
    , Height(hei)
    , Pitch(pit)
    , VideoMemory(mem)
    , BytesPerPixel(bytesPerPixel)
{
    TerminalCoordinates size = this->GetSize();

    for (int x = 0; x < size.X; ++x)
        for (int y = 0; y < size.Y; ++y)
            this->WriteAt(' ', x, y);
}

/*  Writing  */

TerminalWriteResult VbeTerminal::WriteCharAtXy(TerminalBase * const term, const char c, const int16_t cx, const int16_t cy)
{
    VbeTerminal & vterm = *((VbeTerminal *)term);

    /* TEMP */ const uint32_t colf = 0xFFDFE0E6;
    /* TEMP */ const uint32_t colb = 0xFF262223;

    const size_t w = FontWidth;
    const size_t h = FontHeight;
    const size_t x = cx * w;
    const size_t y = cy * h;

    size_t lx, ly, bit;
    size_t line = vterm.VideoMemory + y * vterm.Pitch + x * vterm.BytesPerPixel;
    size_t byteWidth = w / 8;

    if (c == ' ')
    {
        if (colb == NOCOL)
            return {Handle(HandleResult::Okay), 1U, {cx, cy}};

        for (ly = 0; ly < h; ++ly)
        {
            size_t col = 0;

            for (lx = 0; lx < byteWidth; ++lx)
                for (bit = 0; bit < 8; ++bit)
                {
                    *((uint32_t *)(col + line)) = colb;

                    col += vterm.BytesPerPixel;
                }

            line += vterm.Pitch;
        }
    }
    else
    {
        const uint8_t * bmp = Font[c - FontMin];

        for (ly = 0; ly < h; ++ly)
        {
            size_t ind = ly * w / 8, col = 0;

            for (lx = 0; lx < byteWidth; ++lx)
                for (bit = 0; bit < 8; ++bit)
                {
                    if (bmp[ind + lx] & (0x80 >> bit))
                    {
                        if (colf == INVCOL)
                            *((uint32_t *)(col + line)) = ~*((uint32_t *)(col + line));
                        else if (colf != NOCOL)
                            *((uint32_t *)(col + line)) = colf;
                    }
                    else if (colb != NOCOL)
                        *((uint32_t *)(col + line)) = colb;

                    col += vterm.BytesPerPixel;
                }

            line += vterm.Pitch;
        }
    }

    return {Handle(HandleResult::Okay), 1U, {cx, cy}};
}

/*  Positioning  */

TerminalCoordinates VbeTerminal::VbeGetSize(TerminalBase * const term)
{
    VbeTerminal & vterm = *((VbeTerminal *)term);

    return {(int16_t)((int32_t)vterm.Width / (int32_t)FontWidth), (int16_t)((int32_t)vterm.Height / (int32_t)FontHeight)};
}

/*  Remapping  */

void VbeTerminal::RemapMemory(uintptr_t newAddr)
{
    this->VideoMemory = newAddr;
}
