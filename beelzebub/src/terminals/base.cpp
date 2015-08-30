#include <terminals/base.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

/*************************
    TerminalBase class
*************************/

/*  STATICS  */

/*  Writing  */

TerminalWriteResult TerminalBase::DefaultWriteCharAtXy(TerminalBase * const term, const char c, const int16_t x, const int16_t y)
{
    return {Handle(HandleResult::NotImplemented), 0U, InvalidCoordinates};
}

TerminalWriteResult TerminalBase::DefaultWriteCharAtCoords(TerminalBase * const term, const char c, const TerminalCoordinates pos)
{
    return term->WriteAt(c, pos.X, pos.Y);
}

TerminalWriteResult TerminalBase::DefaultWriteStringAt(TerminalBase * const term, const char * const str, const TerminalCoordinates pos)
{
    if (!term->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};
    //  First of all, the terminal needs to be capable of output.

    TerminalCoordinates size;

    if (term->Descriptor->Capabilities.CanGetBufferSize)
        size = term->GetBufferSize();
    else if (term->Descriptor->Capabilities.CanGetSize)
        size = term->GetSize();
    else
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};
    //  A buffer or window size is required to prevent drawing outside
    //  the boundaries.

    uint16_t tabWidth = TerminalBase::DefaultTabulatorWidth;

    if (term->Descriptor->Capabilities.CanGetTabulatorWidth)
        tabWidth = term->GetTabulatorWidth();
    //  Tabulator width is required for proper handling of \t

    WriteCharAtXyFunc WriteCharAtXy = term->Descriptor->WriteCharAtXy;

    int16_t x = pos.X, y = pos.Y;
    //  These are used for positioning characters.

    //msg("Writing %s at %u2:%u2.%n", str, x, y);

#define NEXTLINE do { \
    if (y == size.Y) \
    { \
        y = 0; \
        for (int16_t _x = 0; _x < size.X; ++_x) \
            WriteCharAtXy(term, ' ', _x, y); \
    } \
} while (false)

    uint32_t i = 0;
    //  Declared here so I know how many characters have been written.

    for (; 0 != str[i]; ++i)
    {
        //  Stop at null, naturally.

        char c = str[i];

        if (c == '\r')
            x = 0; //  Carriage return.
        else if (c == '\n')
        {
            ++y;   //  Line feed does not return carriage.
            NEXTLINE;
        }
        else if (c == '\t')
            x = (x / tabWidth + 1) * tabWidth;
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

            TerminalWriteResult tmp = WriteCharAtXy(term, c, x, y);

            if (!tmp.Result.IsOkayResult())
                return {tmp.Result, i, {x, y}};

            x++;
        }
    }

    if (term->Descriptor->Capabilities.CanSetOutputPosition)
    {
        Handle res = term->SetCurrentPosition(x, y);
    
        return {res, i, {x, y}};
    }

    return {Handle(HandleResult::Okay), i, {x, y}};
}

TerminalWriteResult TerminalBase::DefaultWriteChar(TerminalBase * const term, const char c)
{
    if (!(term->Descriptor->Capabilities.CanOutput
       && term->Descriptor->Capabilities.CanGetOutputPosition))
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    char temp[2] = " ";
    temp[0] = c;

    return term->WriteAt(temp, term->GetCurrentPosition());
}

TerminalWriteResult TerminalBase::DefaultWriteString(TerminalBase * const term, const char * const str)
{
    if (!(term->Descriptor->Capabilities.CanOutput
       && term->Descriptor->Capabilities.CanGetOutputPosition))
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    return term->WriteAt(str, term->GetCurrentPosition());
}

TerminalWriteResult TerminalBase::DefaultWriteStringVarargs(TerminalBase * const term, const char * const fmt, va_list args)
{
    if (!term->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    WriteCharFunc writeChar = term->Descriptor->WriteChar;
    WriteStringFunc writeString = term->Descriptor->WriteString;

    TerminalWriteResult res;
    uint32_t cnt = 0U;

    bool inFormat = false;
    char c;
    const char * fmts = fmt;

    while ((c = *fmts++) != 0)
    {
        if (inFormat)
        {
            if (c == 'u')       //  Unsigned decimal.
            {
                /*  Unsigned decimal integer  */

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
                            TERMTRY1(term->WriteUIntD(val8), res, cnt);
                        }
                        break;

                    case '4':
                    case '2':
                    case '1':
                        /*  Apparently chars and shorts are promoted to integers!  */
                        
                        {
                            uint32_t val4 = va_arg(args, uint32_t);
                            TERMTRY1(term->WriteUIntD(val4), res, cnt);
                        }
                        break;

                    default:
                        return {Handle(HandleResult::FormatBadArgumentSize), cnt, InvalidCoordinates};
                }
            }
            else if (c == 'i')       //  Unsigned decimal.
            {
                /*  Unsigned decimal integer  */

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
                            int64_t val8 = va_arg(args, int64_t);
                            TERMTRY1(term->WriteIntD(val8), res, cnt);
                        }
                        break;

                    case '4':
                    case '2':
                    case '1':
                        /*  Apparently chars and shorts are promoted to integers!  */
                        
                        {
                            int32_t val4 = va_arg(args, int32_t);
                            TERMTRY1(term->WriteIntD(val4), res, cnt);
                        }
                        break;

                    default:
                        return {Handle(HandleResult::FormatBadArgumentSize), cnt, InvalidCoordinates};
                }
            }
            else if (c == 'X')       //  Hexadecimal.
            {
                /*  Unsigned decimal integer  */

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
                            TERMTRY1(term->WriteHex64(val8), res, cnt);
                        }
                        break;

                    case '4':
                        {
                            uint32_t val4 = va_arg(args, uint32_t);
                            TERMTRY1(term->WriteHex32(val4), res, cnt);
                        }
                        break;

                    case '2':
                        /*  Apparently chars and shorts are promoted to integers!  */
                        
                        {
                            uint32_t val2 = va_arg(args, uint32_t);
                            TERMTRY1(term->WriteHex16((uint16_t)val2), res, cnt);
                        }
                        break;

                    case '1':
                        /*  Apparently chars and shorts are promoted to integers!  */
                        
                        {
                            uint32_t val1 = va_arg(args, uint32_t);
                            TERMTRY1(term->WriteHex8((uint8_t)val1), res, cnt);
                        }
                        break;

                    default:
                        return {Handle(HandleResult::FormatBadArgumentSize), cnt, InvalidCoordinates};
                }
            }
            else if (c == 'H')  //  Handle.
            {
                Handle h = va_arg(args, Handle);
                
                TERMTRY1(term->WriteHandle(h), res, cnt);
            }
            else if (c == 'B')  //  Boolean (T/F)
            {
                uint32_t val = va_arg(args, uint32_t);
                
                TERMTRY1(writeChar(term, (bool)val ? 'T' : 'F'), res, cnt);
            }
            else if (c == 'b')  //  Boolean/bit (1/0)
            {
                uint32_t val = va_arg(args, uint32_t);
                
                TERMTRY1(writeChar(term, (bool)val ? '1' : '0'), res, cnt);
            }
            else if (c == 's')  //  String.
            {
                char * str = va_arg(args, char *);
                
                TERMTRY1(writeString(term, str), res, cnt);
            }
            else if (c == 'C')  //  Character from pointer.
            {
                char * chr = va_arg(args, char *);
                
                TERMTRY1(writeChar(term, chr[0]), res, cnt);
            }
            else if (c == 'c')  //  Character.
            {
                uint32_t chr = va_arg(args, uint32_t);
                
                TERMTRY1(writeChar(term, (char)chr), res, cnt);
            }
            else if (c == '#')  //  Get current character count.
            {
                *(va_arg(args, uint32_t *)) = res.Size;
            }
            else if (c == '*')  //  Fill with spaces.
            {
                uint32_t n = va_arg(args, uint32_t);

                TERMTRY2(n, writeChar(term, ' '), res, cnt);
            }
            else if (c == 'n')  //  Newline.
            {
                TERMTRY1(writeString(term, "\r\n"), res, cnt);
            }
            else if (c == '%')
                TERMTRY1(writeChar(term, '%'), res, cnt);
            else
                return {Handle(HandleResult::FormatBadSpecifier), cnt, InvalidCoordinates};

            inFormat = false;
        }
        else if (c == '%')
            inFormat = true;
        else
            TERMTRY1(writeChar(term, c), res, cnt);
    }

    return res;
}

TerminalWriteResult TerminalBase::DefaultWriteStringLine(TerminalBase * const term, const char * const str)
{
    TerminalWriteResult tmp = term->Write(str);

    if (!tmp.Result.IsOkayResult())
        return tmp;

    return term->Write("\r\n");
}

TerminalWriteResult TerminalBase::DefaultWriteStringFormat(TerminalBase * const term, const char * const fmt, ...)
{
    if (!term->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    TerminalWriteResult res;
    va_list args;

    va_start(args, fmt);
    res = term->Descriptor->WriteStringVarargs(term, fmt, args);
    va_end(args);

    return res;
}

/*  Positioning  */


Handle TerminalBase::DefaultSetCursorPositionXy(TerminalBase * const term, const int16_t x, const int16_t y)
{
    return Handle(HandleResult::UnsupportedOperation);
}

Handle TerminalBase::DefaultSetCursorPositionCoords(TerminalBase * const term, const TerminalCoordinates pos)
{
    return term->SetCursorPosition(pos.X, pos.Y);
}

TerminalCoordinates TerminalBase::DefaultGetCursorPosition(TerminalBase * const term)
{
    return InvalidCoordinates;
}


Handle TerminalBase::DefaultSetCurrentPositionXy(TerminalBase * const term, const int16_t x, const int16_t y)
{
    return term->SetCurrentPosition({x, y});
}

Handle TerminalBase::DefaultSetCurrentPositionCoords(TerminalBase * const term, const TerminalCoordinates pos)
{
    if (!term->Descriptor->Capabilities.CanSetOutputPosition)
        return Handle(HandleResult::UnsupportedOperation);

    term->CurrentPosition = pos;

    return Handle(HandleResult::Okay);
}

TerminalCoordinates TerminalBase::DefaultGetCurrentPosition(TerminalBase * const term)
{
    if (!term->Descriptor->Capabilities.CanGetOutputPosition)
        return InvalidCoordinates;

    return term->CurrentPosition;
}


Handle TerminalBase::DefaultSetSizeXy(TerminalBase * const term, const int16_t w, const int16_t h)
{
    return Handle(HandleResult::UnsupportedOperation);
}

Handle TerminalBase::DefaultSetSizeCoords(TerminalBase * const term, const TerminalCoordinates pos)
{
    return term->SetSize(pos.X, pos.Y);
}

TerminalCoordinates TerminalBase::DefaultGetSize(TerminalBase * const term)
{
    return InvalidCoordinates;
}


Handle TerminalBase::DefaultSetBufferSizeXy(TerminalBase * const term, const int16_t w, const int16_t h)
{
    return Handle(HandleResult::UnsupportedOperation);
}

Handle TerminalBase::DefaultSetBufferSizeCoords(TerminalBase * const term, const TerminalCoordinates pos)
{
    return term->SetBufferSize(pos.X, pos.Y);
}

TerminalCoordinates TerminalBase::DefaultGetBufferSize(TerminalBase * const term)
{
    return InvalidCoordinates;
}


Handle TerminalBase::DefaultSetTabulatorWidth(TerminalBase * const term, const uint16_t w)
{
    if (!term->Descriptor->Capabilities.CanSetTabulatorWidth)
        return Handle(HandleResult::UnsupportedOperation);

    term->TabulatorWidth = w;

    return Handle(HandleResult::Okay);
}

uint16_t TerminalBase::DefaultGetTabulatorWidth(TerminalBase * const term)
{
    if (!term->Descriptor->Capabilities.CanGetTabulatorWidth)
        return ~((uint16_t)0);

    return term->TabulatorWidth;
}

/*  DYNAMICS  */

/*  Constructor  */

TerminalBase::TerminalBase(const TerminalDescriptor * const desc)
    : Descriptor(desc)
    , CurrentPosition({0, 0})
    , TabulatorWidth(4)
{

}

/*  Writing  */

TerminalWriteResult TerminalBase::WriteAt(const char c, const int16_t x, const int16_t y)
{
    return this->Descriptor->WriteCharAtXy(this, c, x, y);
}

TerminalWriteResult TerminalBase::WriteAt(const char c, const TerminalCoordinates pos)
{
    return this->Descriptor->WriteCharAtCoords(this, c, pos);
}

TerminalWriteResult TerminalBase::WriteAt(const char * const str, const TerminalCoordinates pos)
{
    return this->Descriptor->WriteStringAtCoords(this, str, pos);
}

TerminalWriteResult TerminalBase::Write(const char c)
{
    return this->Descriptor->WriteChar(this, c);
}

TerminalWriteResult TerminalBase::Write(const char * const str)
{
    return this->Descriptor->WriteString(this, str);
}

TerminalWriteResult TerminalBase::Write(const char * const str, va_list args)
{
    return this->Descriptor->WriteStringVarargs(this, str, args);
}

TerminalWriteResult TerminalBase::WriteLine(const char * const str)
{
    return this->Descriptor->WriteLineString(this, str);
}

TerminalWriteResult TerminalBase::WriteFormat(const char * const fmt, ...)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};
    //  I do the check here to avoid messing with the varargs.

    TerminalWriteResult res;
    va_list args;

    va_start(args, fmt);
    res = this->Descriptor->WriteStringVarargs(this, fmt, args);
    va_end(args);

    return res;
}

/*  Positioning  */

#define I_HATE_REPEATING_MYSELF_1(prop) \
Handle TerminalBase::MCATS2(Set, prop)(const int16_t x, const int16_t y) \
{ \
    return this->Descriptor->MCATS3(Set, prop, Xy)(this, x, y); \
} \
Handle TerminalBase::MCATS2(Set, prop)(const TerminalCoordinates pos) \
{ \
    return this->Descriptor->MCATS3(Set, prop, Coords)(this, pos); \
} \
TerminalCoordinates TerminalBase::MCATS2(Get, prop)() \
{ \
    return this->Descriptor->MCATS2(Get, prop)(this); \
} \

I_HATE_REPEATING_MYSELF_1(CursorPosition)
I_HATE_REPEATING_MYSELF_1(CurrentPosition)
I_HATE_REPEATING_MYSELF_1(Size)
I_HATE_REPEATING_MYSELF_1(BufferSize)


Handle TerminalBase::SetTabulatorWidth(const uint16_t w)
{
    return this->Descriptor->SetTabulatorWidth(this, w);
}

uint16_t TerminalBase::GetTabulatorWidth()
{
    return this->Descriptor->GetTabulatorWidth(this);
}

/*  Utility  */

TerminalWriteResult TerminalBase::WriteHandle(const Handle val)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    char str[22] = "<    | |            >";
    //  <type|global/fatal|result/index>

    const char * const strType = val.GetTypeString();
    const char * const strRes  = val.GetResultString();

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

        for (size_t i = 0; 0 != strRes[i] && i < 12; ++i)
            str2[i] = strRes[i];
    }
    else
    {
        if (val.IsGlobal())
            str[6] = 'G';

        char * const str2 = str + 8;
        const uint64_t ind = val.GetIndex();

        for (size_t i = 0; i < 12; ++i)
        {
            uint8_t nib = (ind >> (i << 2)) & 0xF;

            str2[11 - i] = (nib > 9 ? '7' : '0') + nib;
        }
    }

    return this->Descriptor->WriteString(this, str);
}

TerminalWriteResult TerminalBase::WriteIntD(const int64_t val)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

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

    return this->Descriptor->WriteString(this, str + 20 - i);
}

TerminalWriteResult TerminalBase::WriteUIntD(const uint64_t val)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

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

    return this->Descriptor->WriteString(this, str + 20 - i);
}

TerminalWriteResult TerminalBase::WriteHex8(const uint8_t val)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    char str[3];
    str[2] = '\0';

    for (size_t i = 0; i < 2; ++i)
    {
        uint8_t nib = (val >> (i << 2)) & 0xF;

        str[1 - i] = (nib > 9 ? '7' : '0') + nib;
    }

    return this->Descriptor->WriteString(this, str);
}

TerminalWriteResult TerminalBase::WriteHex16(const uint16_t val)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    char str[5];
    str[4] = '\0';

    for (size_t i = 0; i < 4; ++i)
    {
        uint8_t nib = (val >> (i << 2)) & 0xF;

        str[3 - i] = (nib > 9 ? '7' : '0') + nib;
    }

    return this->Descriptor->WriteString(this, str);
}

TerminalWriteResult TerminalBase::WriteHex32(const uint32_t val)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    char str[9];
    str[8] = '\0';

    for (size_t i = 0; i < 8; ++i)
    {
        uint8_t nib = (val >> (i << 2)) & 0xF;

        str[7 - i] = (nib > 9 ? '7' : '0') + nib;
    }

    return this->Descriptor->WriteString(this, str);
}

TerminalWriteResult TerminalBase::WriteHex64(const uint64_t val)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    char str[17];
    str[16] = '\0';

    for (size_t i = 0; i < 16; ++i)
    {
        uint8_t nib = (val >> (i << 2)) & 0xF;

        str[15 - i] = (nib > 9 ? '7' : '0') + nib;
    }

    return this->Descriptor->WriteString(this, str);
}

TerminalWriteResult TerminalBase::WriteHexDump(const uintptr_t start, const size_t length, const size_t charsPerLine)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    char addrhexstr[sizeof(size_t) * 2 + 1], wordhexstr[5], spaces[11] = "          ";
    addrhexstr[sizeof(size_t) * 2] = '\0'; wordhexstr[4] = '\0';

    //WriteCharFunc writeChar = this->Descriptor->WriteChar;
    WriteStringFunc writeString = this->Descriptor->WriteString;

    TerminalWriteResult res;
    uint32_t cnt;

    for (size_t i = 0; i < length; i += charsPerLine)
    {
        uintptr_t lStart = start + i;

        for (size_t j = 0; j < sizeof(size_t) * 2; ++j)
        {
            uint8_t nib = (lStart >> (j << 2)) & 0xF;

            addrhexstr[sizeof(size_t) * 2 - 1 - j] = (nib > 9 ? '7' : '0') + nib;
        }

        TERMTRY1(writeString(this, addrhexstr), res, cnt);
        TERMTRY1(writeString(this, ": "), res, cnt);

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

            TERMTRY1(writeString(this, spaces + spacesOffset), res, cnt);

            uint16_t val = *(uint16_t *)(lStart + j);

            for (size_t j = 0; j < 4; ++j)
            {
                uint8_t nib = (val >> (j << 2)) & 0xF;

                wordhexstr[j ^ 1] = (nib > 9 ? '7' : '0') + nib;
                //  This may be THE smartest, though. It flips the last
                //  bit, so bytes are big endian but the whole word is not.
            }

            TERMTRY1(writeString(this, wordhexstr), res, cnt);
        }

        TERMTRY1(writeString(this, "\n\r"), res, cnt);
    }

    return res;
}

TerminalWriteResult TerminalBase::WriteHexTable(const uintptr_t start, const size_t length, const size_t charsPerLine, const bool ascii)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    char addrhexstr[sizeof(size_t) * 2 + 1], wordhexstr[4] = "   ";
    addrhexstr[sizeof(size_t) * 2] = '\0';

    WriteCharFunc writeChar = this->Descriptor->WriteChar;
    WriteStringFunc writeString = this->Descriptor->WriteString;

    TerminalWriteResult res = {Handle(HandleResult::Okay), 0U, InvalidCoordinates};
    uint32_t cnt = 0U;

    size_t actualCharsPerLine = charsPerLine;

    if (0 == actualCharsPerLine)
    {
        if (this->Descriptor->Capabilities.CanGetBufferSize)
        {
            TerminalCoordinates size = this->Descriptor->GetBufferSize(this);
    
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
            return {Handle(HandleResult::ArgumentOutOfRange), 0U, InvalidCoordinates};
    }

    for (size_t i = 0; i < length; i += actualCharsPerLine)
    {
        uintptr_t lStart = start + i;

        for (size_t j = 0; j < sizeof(size_t) * 2; ++j)
        {
            uint8_t nib = (lStart >> (j << 2)) & 0xF;

            addrhexstr[sizeof(size_t) * 2 - 1 - j] = (nib > 9 ? '7' : '0') + nib;
        }

        TERMTRY1(writeString(this, addrhexstr), res, cnt);
        TERMTRY1(writeChar(this, ':'), res, cnt);

        for (size_t j = 0; j < actualCharsPerLine; ++j)
        {
            uint8_t val = *(uint8_t *)(lStart + j);

            uint8_t nib = val & 0xF;
            wordhexstr[2] = (nib > 9 ? '7' : '0') + nib;
            nib = (val >> 4) & 0xF;
            wordhexstr[1] = (nib > 9 ? '7' : '0') + nib;

            TERMTRY1(writeString(this, wordhexstr), res, cnt);
        }

        if (ascii)
        {
            TERMTRY1(writeString(this, " |"), res, cnt);

            for (size_t j = 0; j < actualCharsPerLine; ++j)
            {
                uint8_t val = *(uint8_t *)(lStart + j);

                if (val >= 32 && val != 127)
                    TERMTRY1(writeChar(this, val), res, cnt);
                else
                    TERMTRY1(writeChar(this, '.'), res, cnt);
            }

            TERMTRY1(writeString(this, "|\n\r"), res, cnt);
        }
        else
            TERMTRY1(writeString(this, "\n\r"), res, cnt);
    }

    return res;
}

/*********************************
    TerminalCoordinates struct
*********************************/

inline __bland TerminalCoordinates TerminalCoordinates::operator+(const TerminalCoordinates other)
{
    return { (int16_t)(this->X + other.X), (int16_t)(this->Y + other.Y) };
}

inline __bland TerminalCoordinates TerminalCoordinates::operator-(const TerminalCoordinates other)
{
    return { (int16_t)(this->X - other.X), (int16_t)(this->Y - other.Y) };
}
