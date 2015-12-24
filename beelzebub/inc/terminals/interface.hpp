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

#include <metaprogramming.h>
#include <handles.h>

#include "stdarg.h"

namespace Beelzebub { namespace Terminals
{
    /**
     * Forward declaration...
     */
    class TerminalBase;

    /**
     * Known major terminal types.
     **/
    enum class TerminalType : uint8_t
    {
        Unknown     = 0x00,

        TextMatrix  = 0x01,
        PixelMatrix = 0x02,

        Serial      = 0x10,

        Keyboard    = 0xFF,
    };

    /**
     * Represents a set of terminal capabilities.
     **/
    struct TerminalCapabilities
    {
        bool CanOutput;            //  Characters can be written to the terminal.
        bool CanInput;             //  Characters can be received from the terminal.
        bool CanRead;              //  Characters can be read back from the terminal's output.

        bool CanGetOutputPosition; //  Position of next output character can be retrieved.
        bool CanSetOutputPosition; //  Position of output characters can be set arbitrarily.
        bool CanPositionCursor;    //  Terminal features a positionable cursor.

        bool CanGetSize;           //  Terminal (window) size can be retrieved.
        bool CanSetSize;           //  Terminal (window) size can be changed.

        bool Buffered;             //  Terminal acts as a window over a buffer.
        bool CanGetBufferSize;     //  Buffer size can be retrieved.
        bool CanSetBufferSize;     //  Buffer size can be changed.
        bool CanPositionWindow;    //  The "window" can be positioned arbitrarily over the buffer.

        bool CanColorBackground;   //  Area behind/around output characters can be colored.
        bool CanColorForeground;   //  Output characters can be colored.
        bool FullColor;            //  32-bit BGRA, or ARGB in little endian.
        bool ForegroundAlpha;      //  Alpha channel of foreground color is supported. (ignored if false)
        bool BackgroundAlpha;      //  Alpha channel of background color is supported. (ignored if false)

        bool CanBold;              //  Output characters can be made bold.
        bool CanUnderline;         //  Output characters can be underlined.
        bool CanBlink;             //  Output characters can blink.

        bool CanGetStyle;          //  Current style settings can be retrieved.

        bool CanGetTabulatorWidth; //  Tabulator width may be retrieved.
        bool CanSetTabulatorWidth; //  Tabulator width may be changed.
        
        bool SequentialOutput;     //  Character sequences can be output without explicit position.

        bool SupportsTitle;        //  Supports assignment of a title.

        TerminalType Type;         //  The known type of the terminal.
    };

    /**
     * Represents a set of coordinates within a terminal's matrix.
     * They can represent a position or a size.
     **/
    struct TerminalCoordinates
    {
        int16_t X;
        int16_t Y;

        TerminalCoordinates operator +(const TerminalCoordinates other);
        TerminalCoordinates operator -(const TerminalCoordinates other);
    };

    const TerminalCoordinates InvalidCoordinates = { -42, 9001 };

    /**
     * Represents the result of a write operation.
     **/
    struct TerminalWriteResult
    {
        Handle Result;
        uint32_t Size;
        TerminalCoordinates End;

        inline TerminalWriteResult()
            : Result(HandleResult::Okay)
            , Size(0U)
            , End({0, 0})
        {

        }

        inline TerminalWriteResult(Handle const res
                                 , uint32_t const size
                                 , TerminalCoordinates const coords)
            : Result(res)
            , Size(size)
            , End(coords)
        {

        }
    };

    /*  Function pointer definition  */

    typedef TerminalWriteResult (*WriteCharAtXyFunc)(TerminalBase * const, const char, const int16_t, const int16_t);
    typedef TerminalWriteResult (*WriteCharAtCoordsFunc)(TerminalBase * const, const char, const TerminalCoordinates);
    typedef TerminalWriteResult (*WriteStringAtCoordsFunc)(TerminalBase * const, const char * const, const TerminalCoordinates);

    typedef TerminalWriteResult (*WriteCharFunc)(TerminalBase * const, const char);
    typedef TerminalWriteResult (*WriteStringFunc)(TerminalBase * const, const char * const);
    typedef TerminalWriteResult (*WriteStringVarargsFunc)(TerminalBase * const, const char * const, va_list);

    typedef Handle (*SetXyFunc)(TerminalBase * const, const int16_t, const int16_t);
    typedef Handle (*SetCoordsFunc)(TerminalBase * const, const TerminalCoordinates);
    typedef TerminalCoordinates (*GetCoords)(TerminalBase * const);

    typedef Handle (*SetUint16)(TerminalBase * const, const uint16_t);
    typedef uint16_t (*GetUint16)(TerminalBase * const);

    /**
     * Describes and defines all the capabilities of a terminal.
     **/
    class TerminalDescriptor
    {
    public:

        /*  Writing  */

        WriteCharAtXyFunc WriteCharAtXy;
        WriteCharAtCoordsFunc WriteCharAtCoords;
        WriteStringAtCoordsFunc WriteStringAtCoords;

        WriteCharFunc WriteChar;
        WriteStringFunc WriteString;
        WriteStringVarargsFunc WriteStringVarargs;
        WriteStringFunc WriteLineString;

        /*  Positioning  */

        SetXyFunc SetCursorPositionXy;
        SetCoordsFunc SetCursorPositionCoords;
        GetCoords GetCursorPosition;

        SetXyFunc SetCurrentPositionXy;
        SetCoordsFunc SetCurrentPositionCoords;
        GetCoords GetCurrentPosition;

        SetXyFunc SetSizeXy;
        SetCoordsFunc SetSizeCoords;
        GetCoords GetSize;

        SetXyFunc SetBufferSizeXy;
        SetCoordsFunc SetBufferSizeCoords;
        GetCoords GetBufferSize;

        SetUint16 SetTabulatorWidth;
        GetUint16 GetTabulatorWidth;

        /*  Capabilities  */

        TerminalCapabilities Capabilities;
    };
}}
