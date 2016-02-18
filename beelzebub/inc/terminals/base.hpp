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

#include <terminals/interface.hpp>
#include <metaprogramming.h>
#include <handles.h>

#include "stdarg.h"

#define TERMTRY0(call, tres) do {           \
tres = call;                                \
if unlikely(!tres.Result.IsOkayResult())    \
    return tres;                            \
} while (false)

#define TERMTRY1(call, tres, cnt) do {      \
cnt = tres.Size;                            \
tres = call;                                \
if likely(tres.Result.IsOkayResult())       \
    tres.Size += cnt;                       \
else                                        \
    return tres;                            \
} while (false)

#define TERMTRY1Ex(call, tres, cnt) do {    \
cnt = tres.Size;                            \
tres = call;                                \
if likely(tres.Result.IsOkayResult())       \
    tres.Size = cnt + 1;                    \
else                                        \
    return tres;                            \
} while (false)

#define TERMTRY2(n, call, tres, cnt) do {               \
cnt = tres.Size;                                        \
for (typeof(n) i = 0; i < n; ++i)                       \
    if unlikely(!(tres = call).Result.IsOkayResult())   \
        return tres;                                    \
tres.Size += cnt + n;                                   \
} while (false)

namespace Beelzebub { namespace Terminals
{
    /**
     * Represents a set of terminal capabilities.
     **/
    class TerminalBase
    {
    public:

        /*  STATICS  */

        static const uint16_t DefaultTabulatorWidth = 8;

        /*  Writing  */

        static TerminalWriteResult DefaultWriteCharAtXy(TerminalBase * const term, const char * c, const int16_t x, const int16_t y);
        static TerminalWriteResult DefaultWriteCharAtCoords(TerminalBase * const term, const char * c, const TerminalCoordinates pos);
        static TerminalWriteResult DefaultWriteStringAt(TerminalBase * const term, const char * const str, const TerminalCoordinates pos);

        static TerminalWriteResult DefaultWriteChar(TerminalBase * const term, const char * c);
        static TerminalWriteResult DefaultWriteString(TerminalBase * const term, const char * const str);
        static TerminalWriteResult DefaultWriteStringVarargs(TerminalBase * const term, const char * const fmt, va_list args);
        static TerminalWriteResult DefaultWriteStringLine(TerminalBase * const term, const char * const str);
        static TerminalWriteResult DefaultWriteStringFormat(TerminalBase * const term, const char * const fmt, ...);

        /*  Positioning  */

        static Handle DefaultSetCursorPositionXy(TerminalBase * const term, const int16_t x, const int16_t y);
        static Handle DefaultSetCursorPositionCoords(TerminalBase * const term, const TerminalCoordinates pos);
        static TerminalCoordinates DefaultGetCursorPosition(TerminalBase * const term);

        static Handle DefaultSetCurrentPositionXy(TerminalBase * const term, const int16_t x, const int16_t y);
        static Handle DefaultSetCurrentPositionCoords(TerminalBase * const term, const TerminalCoordinates pos);
        static TerminalCoordinates DefaultGetCurrentPosition(TerminalBase * const term);

        static Handle DefaultSetSizeXy(TerminalBase * const term, const int16_t w, const int16_t h);
        static Handle DefaultSetSizeCoords(TerminalBase * const term, const TerminalCoordinates pos);
        static TerminalCoordinates DefaultGetSize(TerminalBase * const term);

        static Handle DefaultSetBufferSizeXy(TerminalBase * const term, const int16_t w, const int16_t h);
        static Handle DefaultSetBufferSizeCoords(TerminalBase * const term, const TerminalCoordinates pos);
        static TerminalCoordinates DefaultGetBufferSize(TerminalBase * const term);

        static Handle DefaultSetTabulatorWidth(TerminalBase * const term, const uint16_t w);
        static uint16_t DefaultGetTabulatorWidth(TerminalBase * const term);

        /*  Styling  */

        // ... sooooon.
        
        /*  DYNAMICS  */

        /*  Descriptor  */

        const TerminalDescriptor * Descriptor;

        /*  Constructor  */

        TerminalBase(const TerminalDescriptor * const desc);

        /*  Writing  */

        TerminalWriteResult WriteAt(const char c, const int16_t x, const int16_t y);
        TerminalWriteResult WriteAt(const char c, const TerminalCoordinates pos);
        TerminalWriteResult WriteAt(const char * const str, const TerminalCoordinates pos);

        TerminalWriteResult Write(const char c);
        TerminalWriteResult Write(const char * const str);
        TerminalWriteResult Write(const char * const fmt, va_list args);
        TerminalWriteResult WriteLine(const char * const str);
        __noinline TerminalWriteResult WriteFormat(const char * const fmt, ...);

        /*  Positioning  */

        Handle SetCursorPosition(const int16_t x, const int16_t y);
        Handle SetCursorPosition(const TerminalCoordinates pos);
        TerminalCoordinates GetCursorPosition();

        Handle SetCurrentPosition(const int16_t x, const int16_t y);
        Handle SetCurrentPosition(const TerminalCoordinates pos);
        TerminalCoordinates GetCurrentPosition();

        Handle SetSize(const int16_t w, const int16_t h);
        Handle SetSize(const TerminalCoordinates pos);
        TerminalCoordinates GetSize();

        Handle SetBufferSize(const int16_t w, const int16_t h);
        Handle SetBufferSize(const TerminalCoordinates pos);
        TerminalCoordinates GetBufferSize();

        Handle SetTabulatorWidth(const uint16_t w);
        uint16_t GetTabulatorWidth();

        /*  Utilitary methods  */

        TerminalWriteResult WriteHandle(const Handle val);

        TerminalWriteResult WriteIntD(const int64_t val);
        TerminalWriteResult WriteUIntD(const uint64_t val);

        TerminalWriteResult WriteHex8(const uint8_t val);
        TerminalWriteResult WriteHex16(const uint16_t val);
        TerminalWriteResult WriteHex32(const uint32_t val);
        TerminalWriteResult WriteHex64(const uint64_t val);

        TerminalWriteResult WriteHexDump(const uintptr_t start, const size_t length, const size_t charsPerLine);
        __forceinline TerminalWriteResult WriteHexDump(const void * const start, const size_t length, const size_t charsPerLine)
        {
            return this->WriteHexDump((uintptr_t)start, length, charsPerLine);
        }

        TerminalWriteResult WriteHexTable(const uintptr_t start, const size_t length, const size_t charsPerLine, const bool ascii);
        __forceinline TerminalWriteResult WriteHexTable(const void * const start, const size_t length, const size_t charsPerLine, const bool ascii)
        {
            return this->WriteHexTable((uintptr_t)start, length, charsPerLine, ascii);
        }

        __forceinline TerminalWriteResult WriteLine()
        {
            return this->WriteLine("");
        }

    protected:

        TerminalCoordinates CurrentPosition;
        uint16_t TabulatorWidth;
        bool Overflown;
    };
}}
