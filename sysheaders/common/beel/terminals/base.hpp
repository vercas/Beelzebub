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

#include <beel/terminals/interface.hpp>

#include <stdarg.h>

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
    class TerminalBase;

    typedef TerminalBase & (* TerminalModifier)(TerminalBase & term);

    template<typename TArg>
    __shared_cpp_inline TerminalBase & operator <<(TerminalBase & term, TArg const value);

    enum class TerminalIntegerBase : uint16_t
    {
        Decimal = 10,
        Hexadecimal = 16,
    };

    struct TerminalFormatState
    {
        /*  Constructors  */

        inline constexpr TerminalFormatState()
            : IntegerBase(TerminalIntegerBase::Decimal)
            , ShowPlus(false)
            , NumericUppercase(true)
        {

        }

        /*  Fields  */

        TerminalIntegerBase IntegerBase;
        bool ShowPlus;
        bool NumericUppercase;
    };

    class __public TerminalBase
    {
    public:

        /*  Statics  */

        static uint16_t const DefaultTabulatorWidth = 8;

        /*  Constructor  */

        inline constexpr TerminalBase(TerminalCapabilities const * const caps)
            : Capabilities( caps)
            , CurrentPosition({0, 0})
            , TabulatorWidth(DefaultTabulatorWidth)
            , Overflown(false)
            , FormatState()
        {

        }

        /*  Writing  */

        virtual TerminalWriteResult WriteUtf8At(char const * const c, int16_t const x, int16_t const y);
        virtual TerminalWriteResult WriteUtf8(char const * const c);

        TerminalWriteResult WriteAt(char const c, int16_t const x, int16_t const y);
        TerminalWriteResult WriteAt(char const c, TerminalCoordinates const pos);
        virtual TerminalWriteResult WriteAt(char const * const str, TerminalCoordinates const pos, size_t len = SIZE_MAX);

        virtual TerminalWriteResult Write(char const c);
        virtual TerminalWriteResult Write(char const * const str, size_t len = SIZE_MAX);
        virtual TerminalWriteResult Write(char const * const fmt, va_list args);
        virtual TerminalWriteResult WriteLine(char const * const str, size_t len = SIZE_MAX);
        __solid __min_float TerminalWriteResult WriteFormat(char const * const fmt, ...);

        virtual Handle Flush();

        /*  Positioning  */

        Handle SetCursorPosition(int16_t const x, int16_t const y);
        virtual Handle SetCursorPosition(TerminalCoordinates const pos);
        virtual TerminalCoordinates GetCursorPosition();

        Handle SetCurrentPosition(int16_t const x, int16_t const y);
        virtual Handle SetCurrentPosition(TerminalCoordinates const pos);
        virtual TerminalCoordinates GetCurrentPosition();

        Handle SetSize(int16_t const w, int16_t const h);
        virtual Handle SetSize(TerminalCoordinates const pos);
        virtual TerminalCoordinates GetSize();

        Handle SetBufferSize(int16_t const w, int16_t const h);
        virtual Handle SetBufferSize(TerminalCoordinates const pos);
        virtual TerminalCoordinates GetBufferSize();

        virtual Handle SetTabulatorWidth(uint16_t const w);
        virtual uint16_t GetTabulatorWidth();

        /*  Utilitary methods  */

        virtual TerminalWriteResult WriteHandle(const Handle val);

        virtual TerminalWriteResult WriteIntD(int64_t const val);
        virtual TerminalWriteResult WriteUIntD(uint64_t const val);

        virtual TerminalWriteResult WriteHex8 (uint8_t  const val, bool const upper = true);
        virtual TerminalWriteResult WriteHex16(uint16_t const val, bool const upper = true);
        virtual TerminalWriteResult WriteHex24(uint32_t const val, bool const upper = true);
        virtual TerminalWriteResult WriteHex32(uint32_t const val, bool const upper = true);
        virtual TerminalWriteResult WriteHex48(uint64_t const val, bool const upper = true);
        virtual TerminalWriteResult WriteHex64(uint64_t const val, bool const upper = true);

        virtual __min_float TerminalWriteResult WriteHexFloat(float const val, bool const upper = true);
        virtual __min_float TerminalWriteResult WriteHexDouble(double const val, bool const upper = true);

        virtual TerminalWriteResult WriteHexDump(uintptr_t const start, size_t const length, size_t const charsPerLine);
        __forceinline TerminalWriteResult WriteHexDump(void const * const start, size_t const length, size_t const charsPerLine)
        {
            return this->WriteHexDump((uintptr_t)start, length, charsPerLine);
        }

        virtual TerminalWriteResult WriteHexTable(uintptr_t const start, size_t const length, size_t const charsPerLine, bool const ascii);
        __forceinline TerminalWriteResult WriteHexTable(void const * const start, size_t const length, size_t const charsPerLine, bool const ascii)
        {
            return this->WriteHexTable((uintptr_t)start, length, charsPerLine, ascii);
        }

        __forceinline TerminalWriteResult WriteLine()
        {
            return this->WriteLine("", 1);
        }

        /*  Fields  */

    public:
        TerminalCapabilities const * const Capabilities;

    protected:
        TerminalCoordinates CurrentPosition;
        uint16_t TabulatorWidth;
        bool Overflown;

    public:
        TerminalFormatState FormatState;
    };

    __shared_cpp_inline TerminalBase & EndLine(TerminalBase & term);

    __shared_cpp_inline TerminalBase & Decimal(TerminalBase & term);
    __shared_cpp_inline TerminalBase & Hexadecimal(TerminalBase & term);

    __shared_cpp_inline TerminalBase & ShowPlus(TerminalBase & term);
    __shared_cpp_inline TerminalBase & HidePlus(TerminalBase & term);

    __shared_cpp_inline TerminalBase & NumericUppercase(TerminalBase & term);
    __shared_cpp_inline TerminalBase & NumericLowercase(TerminalBase & term);
}}
