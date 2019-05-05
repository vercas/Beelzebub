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

#include <beel/terminals/base.hpp>
#include <beel/string.hpp>
#include <string.h>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

/*************************
    TerminalBase class
*************************/

/*  Constructor  */

// TerminalBase::TerminalBase(const TerminalCapabilities * const caps)
//     : Capabilities(caps)
//     , CurrentPosition({0, 0})
//     , TabulatorWidth(DefaultTabulatorWidth)
//     , Overflown(false)
//     , FormatState()
// {

// }

/*  Writing  */

TerminalWriteResult TerminalBase::WriteUtf8At(char const * c, int16_t const x, int16_t const y)
{
    (void)c;
    (void)x;
    (void)y;

    return {HandleResult::NotImplemented, 0U, InvalidCoordinates};
}

TerminalWriteResult TerminalBase::WriteUtf8(char const * c)
{
    if (!(this->Capabilities->CanOutput
       && this->Capabilities->CanGetOutputPosition))
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char temp[7] = "\0\0\0\0\0\0";
    temp[0] = *c;
    //  6 bytes + null terminator in UTF-8 character.

    if ((*c & 0xC0) == 0xC0)
    {
        size_t i = 1;

        do
        {
            temp[i] = c[i];

            ++i;
        } while ((c[i] & 0xC0) == 0x80 && i < 7);

        //  This copies the remainder of the bytes, up to 6.
    }

    return this->WriteAt(temp, this->GetCurrentPosition());
}

TerminalWriteResult TerminalBase::WriteAt(char const c, int16_t const x, int16_t const y)
{
    return this->WriteUtf8At(&c, x, y);
}

TerminalWriteResult TerminalBase::WriteAt(char const c, TerminalCoordinates const pos)
{
    return this->WriteUtf8At(&c, pos.X, pos.Y);
}

TerminalWriteResult TerminalBase::WriteAt(char const * const str, TerminalCoordinates const pos, size_t len)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};
    //  First of all, the terminal needs to be capable of output.

    TerminalCoordinates size;

    if (this->Capabilities->CanGetBufferSize)
        size = this->GetBufferSize();
    else if (this->Capabilities->CanGetSize)
        size = this->GetSize();
    else
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};
    //  A buffer or window size is required to prevent drawing outside
    //  the boundaries.

    uint16_t tabWidth = TerminalBase::DefaultTabulatorWidth;

    if (this->Capabilities->CanGetTabulatorWidth)
        tabWidth = this->GetTabulatorWidth();
    //  Tabulator width is required for proper handling of \t

    int16_t x = pos.X, y = pos.Y;
    //  These are used for positioning characters.

    //msg("Writing %s at %u2:%u2.%n", str, x, y);

#define NEXTLINE do {                           \
    if (y == size.Y)                            \
    {                                           \
        this->Overflown = true;                 \
        y = 0;                                  \
        for (int16_t _x = 0; _x < size.X; ++_x) \
            this->WriteUtf8At(" ", _x, y);      \
    }                                           \
    else if (this->Overflown)                   \
        for (int16_t _x = 0; _x < size.X; ++_x) \
            this->WriteUtf8At(" ", _x, y);      \
} while (false)

    uint32_t i = 0, u = 0;
    //  Declared here so I know how many characters have been written.

    for (; 0 != str[i] && i < len; ++i, ++u)
    {
        //  Stop at null before the end, naturally.

        char c = str[i];

        if (c == '\r')
            x = 0; //  Carriage return.
        else if (c == '\n')
        {
            ++y;   //  Line feed does not return carriage.
            NEXTLINE;
        }
        else if (c == '\t')
        {
            uint16_t diff = tabWidth - (x % tabWidth);

            for (int16_t _x = 0; _x < diff; ++_x)
                this->WriteUtf8At(" ", x + _x, y);

            x += diff;
        }
        else if (c == '\b')
        {
            if (x > 0)
                --x;
            //  Once you go \b, you do actually go back.
        }
        else
        {
            if (x == size.X)
            {  x = 0; y++; NEXTLINE;  }

            TerminalWriteResult tmp = this->WriteUtf8At(str + i, x, y);
            //  str + i = &c;

            if (!tmp.Result.IsOkayResult())
                return {tmp.Result, i, {x, y}};

            i += tmp.Size - 1;
            //  Account for UTF-8 multibyte characters.

            x++;
        }
    }

    if (this->Capabilities->CanSetOutputPosition)
    {
        Handle res = this->SetCurrentPosition(x, y);

        return {res, i, {x, y}};
    }

    return {HandleResult::Okay, u, {x, y}};
}

TerminalWriteResult TerminalBase::Write(char const c)
{
    return this->WriteUtf8(&c);
}

TerminalWriteResult TerminalBase::Write(char const * const str, size_t len)
{
    if (!(this->Capabilities->CanOutput
       && this->Capabilities->CanGetOutputPosition))
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    return this->WriteAt(str, this->GetCurrentPosition(), len);
}

TerminalWriteResult TerminalBase::Write(char const * const fmt, va_list args)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    TerminalWriteResult res {};
    uint32_t cnt = 0U;

    bool inFormat = false;
    char c;
    char const * fmts = fmt, * nakedStart = nullptr;

    while ((c = *fmts++) != '\0')
    {
        if (inFormat)
        {
            if (c == 'u')       //  Unsigned decimal.
            {
                char size = *fmts++;

                switch (size)
                {
                    case 's':
                        size = '0' + sizeof(size_t);
                        break;

                    case 'S':
                        size = '0' + sizeof(psize_t);
                        break;

                    case 'p':
                        size = '0' + sizeof(void *);
                        break;

                    case 'P':
                        size = '0' + sizeof(paddr_t);
                        break;

                    default:
                        break;
                }

                switch (size)
                {
                    case '8':
                    case '6':
                        {
                            uint64_t val8 = va_arg(args, uint64_t);
                            TERMTRY1(this->WriteUIntD(val8), res, cnt);
                        }
                        break;

                    case '4':
                    case '3':
                    case '2':
                    case '1':
                        /*  Apparently chars and shorts are promoted to integers!  */

                        {
                            uint32_t val4 = va_arg(args, uint32_t);
                            TERMTRY1(this->WriteUIntD(val4), res, cnt);
                        }
                        break;

                    default:
                        return {HandleResult::FormatBadArgumentSize, cnt, InvalidCoordinates};
                }
            }
            else if (c == 'i')       //  Signed decimal.
            {
                char size = *fmts++;

                switch (size)
                {
                    case 's':
                        size = '0' + sizeof(size_t);
                        break;

                    case 'S':
                        size = '0' + sizeof(psize_t);
                        break;

                    case 'p':
                        size = '0' + sizeof(void *);
                        break;

                    case 'P':
                        size = '0' + sizeof(paddr_t);
                        break;

                    default:
                        break;
                }

                switch (size)
                {
                    case '8':
                    case '6':
                        {
                            int64_t val8 = va_arg(args, int64_t);
                            TERMTRY1(this->WriteIntD(val8), res, cnt);
                        }
                        break;

                    case '4':
                    case '3':
                    case '2':
                    case '1':
                        {
                            int32_t val4 = va_arg(args, int32_t);
                            TERMTRY1(this->WriteIntD(val4), res, cnt);
                        }
                        break;

                    default:
                        return {HandleResult::FormatBadArgumentSize, cnt, InvalidCoordinates};
                }
            }
            else if (c == 'X' || c == 'x')  //  Hexadecimal, any case.
            {
                bool upper = c == 'X';
                char size = *fmts++;

                switch (size)
                {
                    case 's':
                        size = '0' + sizeof(size_t);
                        break;

                    case 'S':
                        size = '0' + sizeof(psize_t);
                        break;

                    case 'p':
                        size = '0' + sizeof(void *);
                        break;

                    case 'P':
                        size = '0' + sizeof(paddr_t);
                        break;

                    default:
                        break;
                }

                switch (size)
                {
                    case '8':
                        {
                            uint64_t val8 = va_arg(args, uint64_t);
                            TERMTRY1(this->WriteHex64(val8, upper), res, cnt);
                        }
                        break;

                    case '6':
                        {
                            uint64_t val8 = va_arg(args, uint64_t);
                            TERMTRY1(this->WriteHex48(val8, upper), res, cnt);
                        }
                        break;

                    case '4':
                        {
                            uint32_t val4 = va_arg(args, uint32_t);
                            TERMTRY1(this->WriteHex32(val4, upper), res, cnt);
                        }
                        break;

                    case '3':
                        {
                            uint32_t val4 = va_arg(args, uint32_t);
                            TERMTRY1(this->WriteHex24(val4, upper), res, cnt);
                        }
                        break;

                    case '2':
                        {
                            uint32_t val2 = va_arg(args, uint32_t);
                            TERMTRY1(this->WriteHex16((uint16_t)val2, upper), res, cnt);
                        }
                        break;

                    case '1':
                        {
                            uint32_t val1 = va_arg(args, uint32_t);
                            TERMTRY1(this->WriteHex8((uint8_t)val1, upper), res, cnt);
                        }
                        break;

                    case 'f':
                    case 'd':
                        {
                            auto func = [&args, this, upper]() __fancy
                            {
                                double valF = va_arg(args, double);
                                return this->WriteHexDouble(valF, upper);
                            };

                            TERMTRY1(func(), res, cnt);
                        }
                        break;

                    default:
                        return {HandleResult::FormatBadArgumentSize, cnt, InvalidCoordinates};
                }
            }
            else if (c == 'H')  //  Handle.
            {
                Handle h = va_arg(args, Handle);

                TERMTRY1(this->WriteHandle(h), res, cnt);
            }
            else if (c == 'B')  //  Boolean (T/F)
            {
                uint32_t val = va_arg(args, uint32_t);

                TERMTRY1(this->WriteUtf8((bool)val ? "T" : "F"), res, cnt);
            }
            else if (c == 'b')  //  Boolean/bit (1/0)
            {
                uint32_t val = va_arg(args, uint32_t);

                TERMTRY1(this->WriteUtf8((bool)val ? "1" : "0"), res, cnt);
            }
            else if (c == 't')  //  Tick (X/ )
            {
                uint32_t val = va_arg(args, uint32_t);

                TERMTRY1(this->WriteUtf8((bool)val ? "X" : " "), res, cnt);
            }
            else if (c == 's')  //  String.
            {
                char * str = va_arg(args, char *);

                TERMTRY1(this->Write(str), res, cnt);
            }
            else if (c == 'S')  //  Solid String.
            {
                size_t len = va_arg(args, size_t);
                char * str = va_arg(args, char *);

                for (size_t i = 0; i < len; ++i)
                    TERMTRY1(this->WriteUtf8(str + i), res, cnt);
            }
            else if (c == 'C')  //  Character from pointer.
            {
                char * chr = va_arg(args, char *);

                TERMTRY1Ex(this->WriteUtf8(chr), res, cnt);
            }
            else if (c == 'c')  //  Character.
            {
                char const chr = (char)(va_arg(args, uint32_t));

                TERMTRY1(this->WriteUtf8(&chr), res, cnt);
            }
            else if (c == '#')  //  Get current character count.
            {
                size_t * const ptr = va_arg(args, size_t *);

                *ptr = res.Size;
            }
            else if (c == '*')  //  Fill with spaces.
            {
                size_t const n = va_arg(args, size_t);

                cnt = res.Size;

                for (size_t i = n; i > 0; --i)
                {
                    res = this->WriteUtf8(" ");

                    if (!res.Result.IsOkayResult())
                        return res;
                }

                res.Size = cnt + (uint32_t)n;
            }
            else if (c == 'n')  //  Newline.
            {
                TERMTRY1(this->Write("\r\n"), res, cnt);
            }
            else if (c == 'F')  //  Flush
            {
                Handle const res2 = this->Flush();

                if unlikely(res2 != HandleResult::Okay && res2 != HandleResult::NotImplemented)
                    return {res2, cnt, res.End};
            }
            else if (c == 'W')  //  Flush & Wait
            {
                Handle const res2 = this->Flush();

                for (size_t volatile i = 0; i < 10'000; ++i) DO_NOTHING();

                if unlikely(res2 != HandleResult::Okay && res2 != HandleResult::NotImplemented)
                    return {res2, cnt, res.End};
            }
            else if (c == '%')
                TERMTRY1(this->WriteUtf8("%"), res, cnt);
            else
                return {HandleResult::FormatBadSpecifier, cnt, InvalidCoordinates};

            inFormat = false;
        }
        else if (c == '%')
        {
            inFormat = true;

            if (nakedStart != nullptr)
            {
                TERMTRY1(this->Write(nakedStart, fmts - 1 - nakedStart), res, cnt);

                nakedStart = nullptr;
            }
        }
        else if unlikely(nakedStart == nullptr)
            nakedStart = fmts - 1;
            //  `fmts - 1` = &c
    }

    if (nakedStart != nullptr)
        TERMTRY1(this->Write(nakedStart), res, cnt);

    return res;
}

TerminalWriteResult TerminalBase::Write(char const * fmt, size_t argc, PositionalFormatArgument const * argv)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    enum { OutOfFormat = 0, ReadingIndex, ReadingFormat } state = OutOfFormat;
    char c;
    size_t argIndex;
    char const * nakedStart = nullptr, * indexStart = nullptr, * formatStart = nullptr;

    TerminalWriteResult res {};
    uint32_t cnt = 0U;

    while ((c = *fmt++) != '\0')
        switch (state)
        {
        case ReadingIndex:
            if (c == '{')
            {
                if (indexStart == fmt - 1)
                {
                    TERMTRY1(this->Write('{'), res, cnt);

                    state = OutOfFormat;
                }
                else
                    return {HandleResult::FormatBadSpecifier, cnt, InvalidCoordinates};
            }
            else if (c == ':' || c == '}')
            {
                if (indexStart == fmt - 1)
                    return {HandleResult::FormatBadSpecifier, cnt, InvalidCoordinates};
                else
                {
                    for (char const * s = indexStart; s < fmt - 1; ++s)
                    {
                        char const d = *s;

                        if (d < '0' || d > '9')
                            return {HandleResult::FormatBadSpecifier, cnt, InvalidCoordinates};

                        argIndex = (argIndex * 10) + (d - '0');
                    }

                    if (argIndex >= argc)
                        return {HandleResult::FormatBadSpecifier, cnt, InvalidCoordinates};

                    if (c == ':')
                    {
                        formatStart = fmt;
                        state = ReadingFormat;
                    }
                    else
                    {
                        state = OutOfFormat;

                        TERMTRY1(argv[argIndex].Writer(this, argv[argIndex].Value, nullptr), res, cnt);
                    }
                }
            }

            break;

        case ReadingFormat:
            if (c == '}')
            {
                char formatBuf[fmt - formatStart] = {};
                strncpy(formatBuf, formatStart, fmt - formatStart - 1);
                
                state = OutOfFormat;

                TERMTRY1(argv[argIndex].Writer(this, argv[argIndex].Value, formatBuf), res, cnt);
            }
            else if (c == '{')
                return {HandleResult::FormatBadSpecifier, cnt, InvalidCoordinates};

            break;

        default:
            if (c == '{')
            {
                if (nakedStart != nullptr)
                {
                    TERMTRY1(this->Write(nakedStart, fmt - 1 - nakedStart), res, cnt);

                    nakedStart = nullptr;
                }

                state = ReadingIndex;
                indexStart = fmt;
                argIndex = 0;
            }
            else if unlikely(nakedStart == nullptr)
                nakedStart = fmt - 1;
                //  `fmt - 1` = &c
            
            break;
        }

    if (state != OutOfFormat)
        return {HandleResult::FormatBadSpecifier, cnt, InvalidCoordinates};

    if (nakedStart != nullptr)
        TERMTRY1(this->Write(nakedStart, fmt - 1 - nakedStart), res, cnt);

    return res;
}

TerminalWriteResult TerminalBase::WriteLine(char const * const str, size_t len)
{
    TerminalWriteResult tmp;
    uint32_t cnt = 0;

    if likely(len > 0)
    {
        tmp = this->Write(str, len);

        if (!tmp.Result.IsOkayResult())
            return tmp;

        cnt = tmp.Size;
    }

    tmp = this->Write("\r\n", 2);

    tmp.Size += cnt;

    return tmp;
}

TerminalWriteResult TerminalBase::WriteFormat(char const * const fmt, ...)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};
    //  I do the check here to avoid messing with the varargs.

    TerminalWriteResult res;
    va_list args;

    va_start(args, fmt);
    res = this->Write(fmt, args);
    va_end(args);

    return res;
}

Handle TerminalBase::Flush()
{
    return HandleResult::NotImplemented;
}

/*  Positioning  */

Handle TerminalBase::SetCursorPosition(int16_t const x, int16_t const y)
{
    (void)x;
    (void)y;

    return HandleResult::UnsupportedOperation;
}

Handle TerminalBase::SetCursorPosition(TerminalCoordinates const pos)
{
    return this->SetCursorPosition(pos.X, pos.Y);
}

TerminalCoordinates TerminalBase::GetCursorPosition()
{
    return InvalidCoordinates;
}


Handle TerminalBase::SetCurrentPosition(int16_t const x, int16_t const y)
{
    return this->SetCurrentPosition({x, y});
}

Handle TerminalBase::SetCurrentPosition(TerminalCoordinates const pos)
{
    if (!this->Capabilities->CanSetOutputPosition)
        return HandleResult::UnsupportedOperation;

    this->CurrentPosition = pos;

    return HandleResult::Okay;
}

TerminalCoordinates TerminalBase::GetCurrentPosition()
{
    if (!this->Capabilities->CanGetOutputPosition)
        return InvalidCoordinates;

    return this->CurrentPosition;
}


Handle TerminalBase::SetSize(int16_t const w, int16_t const h)
{
    (void)w;
    (void)h;

    return HandleResult::UnsupportedOperation;
}

Handle TerminalBase::SetSize(TerminalCoordinates const pos)
{
    return this->SetSize(pos.X, pos.Y);
}

TerminalCoordinates TerminalBase::GetSize()
{
    return InvalidCoordinates;
}


Handle TerminalBase::SetBufferSize(int16_t const w, int16_t const h)
{
    (void)w;
    (void)h;

    return HandleResult::UnsupportedOperation;
}

Handle TerminalBase::SetBufferSize(TerminalCoordinates const pos)
{
    return this->SetBufferSize(pos.X, pos.Y);
}

TerminalCoordinates TerminalBase::GetBufferSize()
{
    return InvalidCoordinates;
}


Handle TerminalBase::SetTabulatorWidth(uint16_t const w)
{
    if (!this->Capabilities->CanSetTabulatorWidth)
        return HandleResult::UnsupportedOperation;

    this->TabulatorWidth = w;

    return HandleResult::Okay;
}

uint16_t TerminalBase::GetTabulatorWidth()
{
    if (!this->Capabilities->CanGetTabulatorWidth)
        return ~((uint16_t)0);

    return this->TabulatorWidth;
}

/*  Utility  */

TerminalWriteResult TerminalBase::WriteHandle(const Handle val)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char str[22] = "<    | |            >";
    //  <type|global/fatal|result/index>

    char const * const strType = val.GetTypeString();

    for (size_t i = 0; 0 != strType[i] && i < 4; ++i)
        str[1 + i] = strType[i];

    if (!val.IsValid())
    {
        str[6] = '-';

        for (size_t i = 8; i < 20; ++i)
            str[i] = '-';
    }
    else if (val.IsType(HandleType::Result))
    {
        if (val.IsFatalResult())
            str[6] = 'F';

        char * const str2 = str + 8;

        char const * const strRes  = val.GetResultString();

        for (size_t i = 0; 0 != strRes[i] && i < 12; ++i)
            str2[i] = strRes[i];
    }
    else
    {
        if (val.IsGlobal())
            str[6] = 'G';

        char * const str2 = str + 8;
        uint64_t const ind = val.GetIndex();

        for (size_t i = 0; i < 12; ++i)
        {
            uint8_t nib = (ind >> (i << 2)) & 0xF;

            str2[11 - i] = (nib > 9 ? '7' : '0') + nib;
        }
    }

    return this->Write(str);
}

TerminalWriteResult TerminalBase::WriteIntD(int64_t const val)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char str[21];
    str[20] = 0;

    size_t i = 0;
    uint64_t x = val > 0 ? (uint64_t)val : ((uint64_t)(-val));

    do
    {
        uint8_t digit = x % 10;

        str[19 - i++] = '0' + digit;

        x /= 10;
    }
    while (x > 0 && i < 20);

    if (val < 0)
        str[19 - i++] = '-';

    return this->Write(str + 20 - i);
}

TerminalWriteResult TerminalBase::WriteUIntD(uint64_t const val)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char str[21];
    str[20] = 0;

    size_t i = 0;
    uint64_t x = val;

    do
    {
        uint8_t digit = x % 10;

        str[19 - i++] = '0' + digit;

        x /= 10;
    }
    while (x > 0 && i < 20);

    return this->Write(str + 20 - i);
}

TerminalWriteResult TerminalBase::WriteHex8(uint8_t const val, bool const upper)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char const alphaBase = (upper ? 'A' : 'a') - 10;

    char str[3];
    str[2] = '\0';

    for (size_t i = 0; i < 2; ++i)
    {
        uint8_t nib = (val >> (i << 2)) & 0xF;

        str[1 - i] = (nib > 9 ? alphaBase : '0') + nib;
    }

    return this->Write(str);
}

TerminalWriteResult TerminalBase::WriteHex16(uint16_t const val, bool const upper)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char const alphaBase = (upper ? 'A' : 'a') - 10;

    char str[5];
    str[4] = '\0';

    for (size_t i = 0; i < 4; ++i)
    {
        uint8_t nib = (val >> (i << 2)) & 0xF;

        str[3 - i] = (nib > 9 ? alphaBase : '0') + nib;
    }

    return this->Write(str);
}

TerminalWriteResult TerminalBase::WriteHex24(uint32_t const val, bool const upper)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char const alphaBase = (upper ? 'A' : 'a') - 10;

    char str[7];
    str[6] = '\0';

    for (size_t i = 0; i < 6; ++i)
    {
        uint8_t nib = (val >> (i << 2)) & 0xF;

        str[5 - i] = (nib > 9 ? alphaBase : '0') + nib;
    }

    return this->Write(str);
}

TerminalWriteResult TerminalBase::WriteHex32(uint32_t const val, bool const upper)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char const alphaBase = (upper ? 'A' : 'a') - 10;

    char str[9];
    str[8] = '\0';

    for (size_t i = 0; i < 8; ++i)
    {
        uint8_t nib = (val >> (i << 2)) & 0xF;

        str[7 - i] = (nib > 9 ? alphaBase : '0') + nib;
    }

    return this->Write(str);
}

TerminalWriteResult TerminalBase::WriteHex48(uint64_t const val, bool const upper)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char const alphaBase = (upper ? 'A' : 'a') - 10;

    char str[13];
    str[12] = '\0';

    for (size_t i = 0; i < 12; ++i)
    {
        uint8_t nib = (val >> (i << 2)) & 0xF;

        str[11 - i] = (nib > 9 ? alphaBase : '0') + nib;
    }

    return this->Write(str);
}

TerminalWriteResult TerminalBase::WriteHex64(uint64_t const val, bool const upper)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char const alphaBase = (upper ? 'A' : 'a') - 10;

    char str[17];
    str[16] = '\0';

    for (size_t i = 0; i < 16; ++i)
    {
        uint8_t nib = (val >> (i << 2)) & 0xF;

        str[15 - i] = (nib > 9 ? alphaBase : '0') + nib;
    }

    return this->Write(str);
}

TerminalWriteResult TerminalBase::WriteHexVar(uint64_t val, bool const upper)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char const alphaBase = (upper ? 'A' : 'a') - 10;

    char str[17];
    str[16] = '\0';
    char * ptr = str + 15;

    do
    {
        uint8_t const nib = val & 0xF;
        val >>= 4;
        
        *ptr-- = (nib > 9 ? alphaBase : '0') + nib;
    } while (val > 0);

    return this->Write(ptr);
}

TerminalWriteResult TerminalBase::WriteHexFloat(float const val, bool const upper)
{
    union { float fVal; uint32_t iVal; } value = {val};

    uint32_t fraction = (value.iVal & 0x007FFFFFU) << 1;
    int32_t exponent = (int32_t)((value.iVal & 0x7F800000U) >> 23) - 127;
    bool negative = (value.iVal >> 31) != 0;

    char const alphaBase = (upper ? 'A' : 'a') - 10;

    char str[] = "-0x1.XxXxXx<<";

    for (size_t i = 0; i < 6; ++i)
    {
        uint8_t const nib = (fraction >> (i << 2)) & 0xF;

        str[10 - i] = (nib > 9 ? alphaBase : '0') + nib;
    }

    TerminalWriteResult res {};
    uint32_t cnt;

    TERMTRY1(this->Write(str + (negative ? 0 : 1)), res, cnt);
    TERMTRY1(this->WriteIntD(exponent), res, cnt);

    return res;
}

TerminalWriteResult TerminalBase::WriteHexDouble(double const val, bool const upper)
{
    union { double fVal; uint64_t iVal; } value = {val};

    uint64_t fraction = value.iVal & 0x000FFFFFFFFFFFFFU;
    int64_t exponent = (int64_t)((value.iVal & 0x7FF0000000000000U) >> 52) - 1023;
    bool negative = (value.iVal >> 63) != 0;

    char const alphaBase = (upper ? 'A' : 'a') - 10;

    char str[] = "-0x1.XxXxXxXxXxXxX<<";

    for (size_t i = 0; i < 13; ++i)
    {
        uint8_t const nib = (fraction >> (i << 2)) & 0xF;

        str[17 - i] = (nib > 9 ? alphaBase : '0') + nib;
    }

    TerminalWriteResult res {};
    uint32_t cnt;

    TERMTRY1(this->Write(str + (negative ? 0 : 1)), res, cnt);
    TERMTRY1(this->WriteIntD(exponent), res, cnt);

    return res;
}

TerminalWriteResult TerminalBase::WriteHexDump(uintptr_t const start, size_t const length, size_t const charsPerLine)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    char addrhexstr[sizeof(size_t) * 2 + 1], wordhexstr[5], spaces[11] = "          ";
    addrhexstr[sizeof(size_t) * 2] = '\0'; wordhexstr[4] = '\0';

    TerminalWriteResult res {};
    uint32_t cnt;

    for (size_t i = 0; i < length; i += charsPerLine)
    {
        uintptr_t lStart = start + i;

        for (size_t j = 0; j < sizeof(size_t) * 2; ++j)
        {
            uint8_t nib = (lStart >> (j << 2)) & 0xF;

            addrhexstr[sizeof(size_t) * 2 - 1 - j] = (nib > 9 ? '7' : '0') + nib;
        }

        TERMTRY1(this->Write(addrhexstr), res, cnt);
        TERMTRY1(this->Write(": "), res, cnt);

        for (size_t j = 0; j < charsPerLine; j += 2)
        {
            size_t spacesOffset = 9;

            if (j > 0)
            {
                if ((j & 0x003) == 0) { --spacesOffset;
                if ((j & 0x007) == 0) { --spacesOffset;
                if ((j & 0x00F) == 0) { --spacesOffset;
                if ((j & 0x01F) == 0) { --spacesOffset;
                if ((j & 0x03F) == 0) { --spacesOffset;
                if ((j & 0x07F) == 0) { --spacesOffset;
                if ((j & 0x0FF) == 0) { --spacesOffset;
                if ((j & 0x1FF) == 0) { --spacesOffset;
                if ((j & 0x3FF) == 0)   --spacesOffset; }}}}}}}}

                //  This is undoubtedly the ugliest hack I've ever written.
            }

            TERMTRY1(this->Write(spaces + spacesOffset), res, cnt);

            uint16_t val = *(uint16_t *)(lStart + j);

            for (size_t k = 0; k < 4; ++k)
            {
                uint8_t nib = (val >> (k << 2)) & 0xF;

                wordhexstr[k ^ 1] = (nib > 9 ? '7' : '0') + nib;
                //  This may be THE smartest, though. It flips the last
                //  bit, so bytes are big endian but the whole word is not.
            }

            TERMTRY1(this->Write(wordhexstr), res, cnt);
        }

        TERMTRY1(this->Write("\n\r"), res, cnt);
    }

    return res;
}

TerminalWriteResult TerminalBase::WriteHexTable(uintptr_t const start, size_t const length, size_t const charsPerLine, bool const ascii)
{
    if (!this->Capabilities->CanOutput)
        return {HandleResult::UnsupportedOperation, 0U, InvalidCoordinates};

    TerminalWriteResult res = {HandleResult::Okay, 0U, InvalidCoordinates};
    uint32_t cnt = 0U;

    size_t actualCharsPerLine = charsPerLine;

    if (0 == actualCharsPerLine)
    {
        if (this->Capabilities->CanGetBufferSize)
        {
            TerminalCoordinates size = this->GetBufferSize();
    
            if (ascii)
            {
                actualCharsPerLine = (size.X - (sizeof(uintptr_t) * 2) - 4) >> 2;
                //  Per line, there is an address and the ':', ' |', '|'.
                //  The latter three make 4 characters.
                //  Besides this, there is a space, two hexadecimal digits and
                //  an ASCII character per byte: 4 chars (thus the >> 2).
            }
            else
            {
                actualCharsPerLine = (size.X - (sizeof(uintptr_t) * 2) - 1) / 3;
                //  Per line, there is an address and the ':'.
                //  The latter three make 1 character.
                //  Besides this, there is a space abd two hexadecimal digits
                //  per byte: 4 chars (thus the >> 2).
            }
        }
        else
            return {HandleResult::ArgumentOutOfRange, 0U, InvalidCoordinates};
    }

    __extension__ char addrhexstr[sizeof(size_t) * 2 + 2], hexstr[actualCharsPerLine * 3 + 1]
       , asciistr[2 + actualCharsPerLine + 4];

    addrhexstr[sizeof(size_t) * 2] = ':';
    addrhexstr[sizeof(size_t) * 2 + 1] = '\0';

    hexstr[actualCharsPerLine * 3] = '\0';

    asciistr[0] = ' ';
    asciistr[1] = '|';
    asciistr[actualCharsPerLine + 2] = '|';
    asciistr[actualCharsPerLine + 3] = '\r';
    asciistr[actualCharsPerLine + 4] = '\n';
    asciistr[actualCharsPerLine + 5] = '\0';

    for (size_t i = 0; i < length; i += actualCharsPerLine)
    {
        uintptr_t lStart = start + i;

        for (size_t j = 0; j < sizeof(size_t) * 2; ++j)
        {
            uint8_t nib = (lStart >> (j << 2)) & 0xF;

            addrhexstr[sizeof(size_t) * 2 - 1 - j] = (nib > 9 ? '7' : '0') + nib;
        }

        TERMTRY1(this->Write(addrhexstr), res, cnt);

        for (size_t j = 0; j < actualCharsPerLine; ++j)
        {
            uint8_t const val = *(uint8_t *)(lStart + j);

            uint8_t nib = val & 0xF;
            hexstr[j * 3 + 2] = (nib > 9 ? '7' : '0') + nib;
            nib = (val >> 4) & 0xF;
            hexstr[j * 3 + 1] = (nib > 9 ? '7' : '0') + nib;
            hexstr[j * 3    ] = ' ';
        }

        TERMTRY1(this->Write(hexstr), res, cnt);

        if (ascii)
        {
            for (size_t j = 0; j < actualCharsPerLine; ++j)
            {
                uint8_t val = *(uint8_t *)(lStart + j);

                if (val >= 32 && val != 127)
                    asciistr[2 + j] = (char)(val);
                else
                    asciistr[2 + j] = '.';
            }

            TERMTRY1(this->Write(asciistr), res, cnt);
        }
        else
            TERMTRY1(this->Write("\n\r"), res, cnt);
    }

    return res;
}

/*  Now to implement the << operator.  */

namespace Beelzebub { namespace Terminals
{
    template<>
    TerminalBase & operator << <bool>(TerminalBase & term, bool const value)
    {
        term.Write(value ? "True" : "False");

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<bool>(TerminalBase * term, bool const * val, char const * format)
    {
        if (format == nullptr || ::streq(format, "Bool"))
            return term->Write(*val ? "True" : "False");
        else if (::streq(format, "bool"))
            return term->Write(*val ? "true" : "false");
        else if (::streq(format, "tick"))
            return term->Write(*val ? 'X' : ' ');
        else if (::streq(format, "bit"))
            return term->Write(*val ? '1' : '0');
        else
            return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
    }
    
    #define SPAWN_INT(size, type) template<> \
    TerminalBase & operator << <type>(TerminalBase & term, type const value) \
    { \
        if (term.FormatState.IntegerBase == TerminalIntegerBase::Decimal) \
        { \
            if unlikely(term.FormatState.ShowPlus && value > (type)0) \
                if unlikely(!term.Write('+').Result.IsOkayResult()) \
                    return term; \
 \
            term.WriteIntD(value); \
        } \
        else if (term.FormatState.IntegerBase == TerminalIntegerBase::Hexadecimal) \
            term.MCATS(WriteHex, size)(static_cast<MCATS(uint, size, _t)>(value), term.FormatState.NumericUppercase); \
 \
        return term; \
    } \
    template<> \
    TerminalWriteResult WriteFormattedValue<type>(TerminalBase * term, type const * val, char const * format) \
    { \
        return WriteFormattedIntHelperEx<type>(term, *val, format, "i"BeH); \
    }

    #define SPAWN_UINT(size, type) template<> \
    TerminalBase & operator << <type>(TerminalBase & term, type const value) \
    { \
        if (term.FormatState.IntegerBase == TerminalIntegerBase::Decimal) \
        { \
            if unlikely(term.FormatState.ShowPlus) \
                if unlikely(!term.Write('+').Result.IsOkayResult()) \
                    return term; \
 \
            term.WriteUIntD(value); \
        } \
        else if (term.FormatState.IntegerBase == TerminalIntegerBase::Hexadecimal) \
            term.MCATS(WriteHex, size)(value, term.FormatState.NumericUppercase); \
 \
        return term; \
    } \
    template<> \
    TerminalWriteResult WriteFormattedValue<type>(TerminalBase * term, type const * val, char const * format) \
    { \
        return WriteFormattedIntHelperEx<type>(term, *val, format, "u"BeH); \
    }

    SPAWN_INT(16, signed short)
    SPAWN_INT(32, signed int)
    SPAWN_INT(64, signed long long)

    SPAWN_UINT( 8, unsigned char)
    SPAWN_UINT(16, unsigned short)
    SPAWN_UINT(32, unsigned int)
    SPAWN_UINT(64, unsigned long long)

#if   defined(__BEELZEBUB__ARCH_AMD64)
    SPAWN_INT(64, signed long)
    SPAWN_UINT(64, unsigned long)
#else
    SPAWN_INT(32, signed long)
    SPAWN_UINT(32, unsigned long)
#endif

    template<>
    TerminalBase & operator << <void const *>(TerminalBase & term, void const * const value)
    {
        uintptr_t const ptr = reinterpret_cast<uintptr_t>(value);

#if   defined(__BEELZEBUB__ARCH_AMD64)
        term.WriteHex64(ptr, term.FormatState.NumericUppercase);
#else
        term.WriteHex32(ptr, term.FormatState.NumericUppercase);
#endif

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<void const *>(TerminalBase * term, void const * const * val, char const * format)
    {
        return WriteFormattedIntHelperEx<uintptr_t>(term, reinterpret_cast<uintptr_t>(*val), format,
#if   defined(__BEELZEBUB__ARCH_AMD64)
            "X8"BeH
#else
            "X4"BeH
#endif
        );
    }

    template<>
    TerminalBase & operator << <void *>(TerminalBase & term, void * const value)
    {
        return term << const_cast<void const *>(value);
    }

    template<>
    TerminalWriteResult WriteFormattedValue<void *>(TerminalBase * term, void * const * val, char const * format)
    {
        return WriteFormattedValue<void const *>(term, val, format);
    }

    template<>
    TerminalBase & operator << <paddr_t>(TerminalBase & term, paddr_t const value)
    {
#if   defined(__BEELZEBUB__ARCH_AMD64)
        term.WriteHex64(value.Value, term.FormatState.NumericUppercase);
#else
        term.WriteHex32(value.Value, term.FormatState.NumericUppercase);
#endif

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<paddr_t>(TerminalBase * term, paddr_t const * val, char const * format)
    {
        return WriteFormattedIntHelperEx<paddr_inner_t>(term, val->Value, format,
#if   defined(__BEELZEBUB__ARCH_AMD64)
            "X8"BeH
#else
            "X4"BeH
#endif
        );
    }

    template<>
    TerminalBase & operator << <vaddr_t>(TerminalBase & term, vaddr_t const value)
    {
#if   defined(__BEELZEBUB__ARCH_AMD64)
        term.WriteHex64(value.Value, term.FormatState.NumericUppercase);
#else
        term.WriteHex32(value.Value, term.FormatState.NumericUppercase);
#endif

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<vaddr_t>(TerminalBase * term, vaddr_t const * val, char const * format)
    {
        return WriteFormattedIntHelperEx<vaddr_inner_t>(term, val->Value, format,
#if   defined(__BEELZEBUB__ARCH_AMD64)
            "X8"BeH
#else
            "X4"BeH
#endif
        );
    }

    template<>
    TerminalBase & operator << <psize_t>(TerminalBase & term, psize_t const value)
    {
        term.WriteHexVar(value.Value, term.FormatState.NumericUppercase);

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<psize_t>(TerminalBase * term, psize_t const * val, char const * format)
    {
        if (format == nullptr)
            return term->WriteHexVar(val->Value, true);
        else
            return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
    }

    template<>
    TerminalBase & operator << <vsize_t>(TerminalBase & term, vsize_t const value)
    {
        term.WriteHexVar(value.Value, term.FormatState.NumericUppercase);

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<vsize_t>(TerminalBase * term, vsize_t const * val, char const * format)
    {
        if (format == nullptr)
            return term->WriteHexVar(val->Value, true);
        else
            return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
    }

    template<>
    TerminalBase & operator << <PageSize_t>(TerminalBase & term, PageSize_t const value)
    {
        term.WriteHexVar(value.Value, term.FormatState.NumericUppercase);

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<PageSize_t>(TerminalBase * term, PageSize_t const * val, char const * format)
    {
        if (format == nullptr)
            return term->WriteHexVar(val->Value, true);
        else
            return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
    }

    template<>
    TerminalBase & operator << <float>(TerminalBase & term, float const value)
    {
        term.WriteHexFloat(value, term.FormatState.NumericUppercase);

        return term;
    }
    
    template<>
    TerminalBase & operator << <double>(TerminalBase & term, double const value)
    {
        term.WriteHexDouble(value, term.FormatState.NumericUppercase);

        return term;
    }
    
    template<>
    TerminalBase & operator << <char>(TerminalBase & term, char const value)
    {
        term.Write(value);

        return term;
    }
    
    template<>
    TerminalBase & operator << <char const *>(TerminalBase & term, char const * const value)
    {
        term.Write(value);

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<char const *>(TerminalBase * term, char const * const * val, char const * format)
    {
        if (format == nullptr)
            return term->Write(*val);
        else
            return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
    }
    
    template<>
    TerminalBase & operator << <char *>(TerminalBase & term, char * const value)
    {
        term.Write(const_cast<char const *>(value));

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<char *>(TerminalBase * term, char * const * val, char const * format)
    {
        if (format == nullptr)
            return term->Write(*val);
        else
            return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
    }
    
    template<>
    TerminalBase & operator << <Handle>(TerminalBase & term, Handle const value)
    {
        term.WriteHandle(value);

        return term;
    }

    template<>
    TerminalWriteResult WriteFormattedValue<Handle>(TerminalBase * term, Handle const * val, char const * format)
    {
        if (format == nullptr)
            return term->WriteHandle(*val);
        else
            return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
    }
    
    // template<>
    // TerminalBase & operator << <HandleResult>(TerminalBase & term, HandleResult const value)
    // {
    //     term.WriteHandle(value);

    //     return term;
    // }

    // template<>
    // TerminalWriteResult WriteFormattedValue<HandleResult>(TerminalBase * term, HandleResult const * val, char const * format)
    // {
    //     Handle const han = *val;

    //     return WriteFormattedValue<Handle>(term, &han, format);
    // }
    
    template<>
    TerminalBase & operator << <TerminalModifier>(TerminalBase & term, TerminalModifier const value)
    {
        return value(term);
    }
}}

TerminalBase & Terminals::EndLine(TerminalBase & term)
{
    term.WriteLine();

    return term;
}

TerminalBase & Terminals::Decimal(TerminalBase & term)
{
    term.FormatState.IntegerBase = TerminalIntegerBase::Decimal;

    return term;
}

TerminalBase & Terminals::Hexadecimal(TerminalBase & term)
{
    term.FormatState.IntegerBase = TerminalIntegerBase::Hexadecimal;

    return term;
}

TerminalBase & Terminals::ShowPlus(TerminalBase & term)
{
    term.FormatState.ShowPlus = true;

    return term;
}

TerminalBase & Terminals::HidePlus(TerminalBase & term)
{
    term.FormatState.ShowPlus = false;

    return term;
}

TerminalBase & Terminals::NumericUppercase(TerminalBase & term)
{
    term.FormatState.NumericUppercase = true;

    return term;
}

TerminalBase & Terminals::NumericLowercase(TerminalBase & term)
{
    term.FormatState.NumericUppercase = false;

    return term;
}

TerminalWriteResult Terminals::WriteFormattedEnumHelper(TerminalBase * term
                                                      , uint64_t val
                                                      , char const * format
                                                      , char const * (*printer)(uint64_t val)
                                                      , size_t sz)
{
    bool upper = true;

    switch (CStringUtils::Hash(format))
    {
    case 0:
    case "s"BeH:
        return term->Write(printer(val));
    case "i"BeH:
        return term->WriteIntD((int64_t)val);
    case "u"BeH:
        return term->WriteUIntD(val);

    case "X1"BeH:
        return term->WriteHex8((uint8_t)val, true);
    case "X2"BeH:
        return term->WriteHex16((uint16_t)val, true);
    case "X4"BeH:
        return term->WriteHex32((uint32_t)val, true);
    case "X6"BeH:
        return term->WriteHex48(val, true);
    case "X8"BeH:
        return term->WriteHex64(val, true);

    case "x1"BeH:
        return term->WriteHex8((uint8_t)val, false);
    case "x2"BeH:
        return term->WriteHex16((uint16_t)val, false);
    case "x4"BeH:
        return term->WriteHex32((uint32_t)val, false);
    case "x6"BeH:
        return term->WriteHex48(val, false);
    case "x8"BeH:
        return term->WriteHex64(val, false);

    case "x"BeH:
        upper = false;
        [[fallthrough]];
    case "X"BeH:    //  Here `upper` is true.
        switch (sz)
        {
        case 1:
            return term->WriteHex8((uint8_t)val, upper);
        case 2:
            return term->WriteHex16((uint16_t)val, upper);
        case 4:
            return term->WriteHex32((uint32_t)val, upper);
        case 6:
            return term->WriteHex48(val, upper);
        case 8:
            return term->WriteHex64(val, upper);
        default:
            return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
        }

    default:
        return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
    }
}

TerminalWriteResult Terminals::WriteFormattedIntHelper(TerminalBase * term
                                                     , uint64_t val
                                                     , char const * format
                                                     , unsigned long long defFmt
                                                     , size_t sz)
{
    bool upper = true;

    switch (format == nullptr ? defFmt : CStringUtils::Hash(format))
    {
    case "i"BeH:
        return term->WriteIntD((int64_t)val);
    case "u"BeH:
        return term->WriteUIntD(val);

    case "X1"BeH:
        return term->WriteHex8((uint8_t)val, true);
    case "X2"BeH:
        return term->WriteHex16((uint16_t)val, true);
    case "X4"BeH:
        return term->WriteHex32((uint32_t)val, true);
    case "X6"BeH:
        return term->WriteHex48(val, true);
    case "X8"BeH:
        return term->WriteHex64(val, true);

    case "x1"BeH:
        return term->WriteHex8((uint8_t)val, false);
    case "x2"BeH:
        return term->WriteHex16((uint16_t)val, false);
    case "x4"BeH:
        return term->WriteHex32((uint32_t)val, false);
    case "x6"BeH:
        return term->WriteHex48(val, false);
    case "x8"BeH:
        return term->WriteHex64(val, false);

    case "x"BeH:
        upper = false;
        [[fallthrough]];
    case "X"BeH:    //  Here `upper` is true.
        switch (sz)
        {
        case 1:
            return term->WriteHex8((uint8_t)val, upper);
        case 2:
            return term->WriteHex16((uint16_t)val, upper);
        case 4:
            return term->WriteHex32((uint32_t)val, upper);
        case 6:
            return term->WriteHex48(val, upper);
        case 8:
            return term->WriteHex64(val, upper);
        default:
            return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
        }

    default:
        return {HandleResult::FormatBadSpecifier, 0, InvalidCoordinates};
    }
}
