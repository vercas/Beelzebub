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

#include "terminals/djinn.hpp"
#include <djinn.h>
#include <string.h>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

/*  Djinn terminal descriptor  */

TerminalCapabilities DjinnTerminalCapabilities = {
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
};

/***************************
    DjinnTerminal struct
***************************/

/*  Constructors    */

DjinnTerminal::DjinnTerminal()
    : TerminalBase(&DjinnTerminalCapabilities)
{

}

/*  Writing  */

static TerminalWriteResult WriteWrapper(char const * str, size_t len, bool & cont)
{
    size_t const n = len;
    uint32_t u = 0;
    cont = false;

    while (len > 0)
    {
        DjinnLogResult res = DjinnLog(str, (int)len);

        switch (res.Result)
        {
        case DJINN_LOG_SUCCESS:
            for (size_t i = 0; str[i] != '\0' && i < res.Count; ++i)
                if likely((str[i] & 0xC0) != 0x80)
                    ++u;

            str += res.Count;
            len -= res.Count;

            break;  //  Keep printing.

        case DJINN_LOG_NO_DEBUGGERS:
            return {HandleResult::Okay, u, InvalidCoordinates};
        default:
            return {HandleResult::Failed, u, InvalidCoordinates};
        }
    }

    cont = true;
    return {HandleResult::Okay, u, InvalidCoordinates};
}

TerminalWriteResult DjinnTerminal::WriteUtf8(char const * c)
{
    size_t i = 1;
    char temp[7] = "\0\0\0\0\0\0";
    temp[0] = *c;
    //  6 bytes + null terminator in UTF-8 character.

    if ((*c & 0xC0) == 0xC0)
    {
        do
        {
            temp[i] = c[i];

            ++i;
        } while ((c[i] & 0xC0) == 0x80 && i < 7);

        //  This copies the remainder of the bytes, up to 6.
    }

    bool dummy;
    return WriteWrapper(&(temp[0]), i, dummy);
}

TerminalWriteResult DjinnTerminal::Write(char const * str, size_t len)
{
    if (len == SIZE_MAX)
        len = strlen(str);

    bool dummy;
    return WriteWrapper(str, len, dummy);
}

TerminalWriteResult DjinnTerminal::WriteLine(char const * str, size_t len)
{
    if (len == SIZE_MAX)
        len = strlen(str);

    bool cont;

    TerminalWriteResult res = WriteWrapper(str, len, cont);

    if (!cont) return res;

    len = res.Size;
    res = WriteWrapper("\r\n", 2, cont);

    if (res.Size == 2) res.Size += (uint32_t)len;

    return res;
}

Handle DjinnTerminal::Flush()
{
    return HandleResult::Okay;
}

/*  Utilitary methods  */

static uint32_t UInt64LenHelper(uint64_t val)
{
    size_t count = 1;

    if (val >= 10000000000000000UL) { count += 16; val /= 10000000000000000UL; }
    if (val >= 100000000U)          { count += 8;  val /= 100000000U;          }
    if (val >= 10000)               { count += 4;  val /= 10000;               }
    if (val >= 100)                 { count += 2;  val /= 100;                 }
    if (val >= 10)                    count += 1;

    return count;
}

static uint32_t Int64LenHelper(int64_t val)
{
    if (val > 0)
        return UInt64LenHelper(static_cast<uint64_t>( val));
    else
        return UInt64LenHelper(static_cast<uint64_t>(-val)) + 1;
}

static uint32_t Hex64LenHelper(uint64_t val)
{
    size_t count = 1;

    if (val >> 32) { count += 8; val >>= 32; }
    if (val >> 16) { count += 4; val >>= 16; }
    if (val >>  8) { count += 2; val >>=  8; }
    if (val >>  4)   count += 1;

    return count;
}

TerminalWriteResult DjinnTerminal::WriteIntD(int64_t val)
{
    DjinnLogResult res = DjinnLogInt(val, DJINN_INT_DEC);

    switch (res.Result)
    {
    case DJINN_LOG_SUCCESS:
    case DJINN_LOG_NO_DEBUGGERS:
        return {HandleResult::Okay, Int64LenHelper(val), InvalidCoordinates};
    case DJINN_LOG_STRINGIFY:
        return this->TerminalBase::WriteIntD(val);
    default:
        return {HandleResult::Failed, 0, InvalidCoordinates};
    }
}

TerminalWriteResult DjinnTerminal::WriteUIntD(uint64_t val)
{
    DjinnLogResult res = DjinnLogUInt(val, DJINN_INT_UDEC);

    switch (res.Result)
    {
    case DJINN_LOG_SUCCESS:
    case DJINN_LOG_NO_DEBUGGERS:
        return {HandleResult::Okay, UInt64LenHelper(val), InvalidCoordinates};
    case DJINN_LOG_STRINGIFY:
        return this->TerminalBase::WriteUIntD(val);
    default:
        return {HandleResult::Failed, 0, InvalidCoordinates};
    }
}

TerminalWriteResult DjinnTerminal::WriteHex8(uint8_t val, bool upper)
{
    DjinnLogResult res = DjinnLogUInt(val, upper ? DJINN_INT_HEX8_U : DJINN_INT_HEX8_L);

    switch (res.Result)
    {
    case DJINN_LOG_SUCCESS:
    case DJINN_LOG_NO_DEBUGGERS:
        return {HandleResult::Okay, 2, InvalidCoordinates};
    case DJINN_LOG_STRINGIFY:
        return this->TerminalBase::WriteHex8(val, upper);
    default:
        return {HandleResult::Failed, 0, InvalidCoordinates};
    }
}

TerminalWriteResult DjinnTerminal::WriteHex16(uint16_t val, bool upper)
{
    DjinnLogResult res = DjinnLogUInt(val, upper ? DJINN_INT_HEX16_U : DJINN_INT_HEX16_L);

    switch (res.Result)
    {
    case DJINN_LOG_SUCCESS:
    case DJINN_LOG_NO_DEBUGGERS:
        return {HandleResult::Okay, 4, InvalidCoordinates};
    case DJINN_LOG_STRINGIFY:
        return this->TerminalBase::WriteHex16(val, upper);
    default:
        return {HandleResult::Failed, 0, InvalidCoordinates};
    }
}

TerminalWriteResult DjinnTerminal::WriteHex24(uint32_t val, bool upper)
{
    DjinnLogResult res = DjinnLogUInt(val, upper ? DJINN_INT_HEX24_U : DJINN_INT_HEX24_L);

    switch (res.Result)
    {
    case DJINN_LOG_SUCCESS:
    case DJINN_LOG_NO_DEBUGGERS:
        return {HandleResult::Okay, 6, InvalidCoordinates};
    case DJINN_LOG_STRINGIFY:
        return this->TerminalBase::WriteHex24(val, upper);
    default:
        return {HandleResult::Failed, 0, InvalidCoordinates};
    }
}

TerminalWriteResult DjinnTerminal::WriteHex32(uint32_t val, bool upper)
{
    DjinnLogResult res = DjinnLogUInt(val, upper ? DJINN_INT_HEX32_U : DJINN_INT_HEX32_L);

    switch (res.Result)
    {
    case DJINN_LOG_SUCCESS:
    case DJINN_LOG_NO_DEBUGGERS:
        return {HandleResult::Okay, 8, InvalidCoordinates};
    case DJINN_LOG_STRINGIFY:
        return this->TerminalBase::WriteHex32(val, upper);
    default:
        return {HandleResult::Failed, 0, InvalidCoordinates};
    }
}

TerminalWriteResult DjinnTerminal::WriteHex48(uint64_t val, bool upper)
{
    DjinnLogResult res = DjinnLogUInt(val, upper ? DJINN_INT_HEX48_U : DJINN_INT_HEX48_L);

    switch (res.Result)
    {
    case DJINN_LOG_SUCCESS:
    case DJINN_LOG_NO_DEBUGGERS:
        return {HandleResult::Okay, 12, InvalidCoordinates};
    case DJINN_LOG_STRINGIFY:
        return this->TerminalBase::WriteHex48(val, upper);
    default:
        return {HandleResult::Failed, 0, InvalidCoordinates};
    }
}

TerminalWriteResult DjinnTerminal::WriteHex64(uint64_t val, bool upper)
{
    DjinnLogResult res = DjinnLogUInt(val, upper ? DJINN_INT_HEX64_U : DJINN_INT_HEX64_L);

    switch (res.Result)
    {
    case DJINN_LOG_SUCCESS:
    case DJINN_LOG_NO_DEBUGGERS:
        return {HandleResult::Okay, 16, InvalidCoordinates};
    case DJINN_LOG_STRINGIFY:
        return this->TerminalBase::WriteHex64(val, upper);
    default:
        return {HandleResult::Failed, 0, InvalidCoordinates};
    }
}

TerminalWriteResult DjinnTerminal::WriteHexVar(uint64_t val, bool upper)
{
    DjinnLogResult res = DjinnLogUInt(val, upper ? DJINN_INT_HEX_VAR_U : DJINN_INT_HEX_VAR_L);

    switch (res.Result)
    {
    case DJINN_LOG_SUCCESS:
    case DJINN_LOG_NO_DEBUGGERS:
        return {HandleResult::Okay, Hex64LenHelper(val), InvalidCoordinates};
    case DJINN_LOG_STRINGIFY:
        return this->TerminalBase::WriteHexVar(val, upper);
    default:
        return {HandleResult::Failed, 0, InvalidCoordinates};
    }
}

