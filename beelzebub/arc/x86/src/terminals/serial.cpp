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

#include <terminals/serial.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

/*  Serial terminal descriptor  */

TerminalDescriptor SerialTerminalDescriptor = {
    &TerminalBase::DefaultWriteCharAtXy,
    &TerminalBase::DefaultWriteCharAtCoords,
    &TerminalBase::DefaultWriteStringAt,
    &SerialTerminal::WriteChar,
    &SerialTerminal::WriteString,
    &TerminalBase::DefaultWriteStringVarargs,
    &SerialTerminal::WriteStringLine,

    &TerminalBase::DefaultSetCursorPositionXy,
    &TerminalBase::DefaultSetCursorPositionCoords,
    &TerminalBase::DefaultGetCursorPosition,

    &TerminalBase::DefaultSetCurrentPositionXy,
    &TerminalBase::DefaultSetCurrentPositionCoords,
    &TerminalBase::DefaultGetCurrentPosition,

    &TerminalBase::DefaultSetSizeXy,
    &TerminalBase::DefaultSetSizeCoords,
    &TerminalBase::DefaultGetSize,

    &TerminalBase::DefaultSetBufferSizeXy,
    &TerminalBase::DefaultSetBufferSizeCoords,
    &TerminalBase::DefaultGetBufferSize,

    &TerminalBase::DefaultSetTabulatorWidth,
    &TerminalBase::DefaultGetTabulatorWidth,

    {
        true,   //  bool CanOutput;            //  Characters can be written to the terminal.
        false,  //  bool CanInput;             //  Characters can be received from the terminal.
        false,  //  bool CanRead;              //  Characters can be read back from the terminal's output.

        false,  //  bool CanGetOutputPosition; //  Position of next output character can be retrieved.
        false,  //  bool CanSetOutputPosition; //  Position of output characters can be set arbitrarily.
        false,  //  bool CanPositionCursor;    //  Terminal features a positionable cursor.

        false,  //  bool CanGetSize;           //  Terminal (window) size can be retrieved.
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

        false,  //  bool CanGetTabulatorWidth; //  Tabulator width may be retrieved.
        false,  //  bool CanSetTabulatorWidth; //  Tabulator width may be changed.
        
        true,   //  bool SequentialOutput;     //  Character sequences can be output without explicit position.

        false,  //  bool SupportsTitle;        //  Supports assignment of a title.

        TerminalType::Serial    //  TerminalType Type;         //  The known type of the terminal.
    }
};

/****************************
    SerialTerminal struct
*****************************/

/*  Constructors    */

SerialTerminal::SerialTerminal(ManagedSerialPort * const port)
    : TerminalBase(&SerialTerminalDescriptor)
    , Port(port)
{
    
}

/*  Writing  */

TerminalWriteResult SerialTerminal::WriteChar(TerminalBase * const term, const char * c)
{
    SerialTerminal * const sterm = (SerialTerminal *)term;

    uint32_t const charBytes = (uint32_t)(sterm->Port->WriteUtf8Char(c));

    return {Handle(HandleResult::Okay), charBytes, InvalidCoordinates};
}

TerminalWriteResult SerialTerminal::WriteString(TerminalBase * const term, const char * const str)
{
    SerialTerminal * const sterm = (SerialTerminal *)term;

    return {Handle(HandleResult::Okay), (uint32_t)sterm->Port->WriteNtString(str), InvalidCoordinates};
}

TerminalWriteResult SerialTerminal::WriteStringLine(TerminalBase * const term, const char * const str)
{
    SerialTerminal * const sterm = (SerialTerminal *)term;

    size_t n = sterm->Port->WriteNtString(str);
    n += sterm->Port->WriteNtString("\r\n");

    return {Handle(HandleResult::Okay), (uint32_t)n, InvalidCoordinates};
}
