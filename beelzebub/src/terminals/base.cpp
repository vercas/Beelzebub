#include <terminals/base.hpp>

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

    int16_t x = pos.X, y = pos.Y;
    //  These are used for positioning characters.

    uint32_t i = 0;
    //  Declared here so I know how many characters have been written.

    for (; 0 != str[i]; ++i)
    {
        //  Stop at null, naturally.

        char c = str[i];

        if (c == '\r')
            x = 0; //  Carriage return.
        else if (c == '\n')
            ++y;   //  Line feed does not return carriage.
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
            {  x = 0; y++;  }

            TerminalWriteResult tmp = term->WriteAt(c, x, y);

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

    return term->WriteAt(c, term->GetCurrentPosition());
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
    uint32_t cnt;

    bool inFormat = false;
    char c;
    const char * fmts = fmt;

    while ((c = *fmts++) != 0)
    {
        if (inFormat)
        {
            if (c == 'u')
            {
                /*  Unsigned decimal integer  */

                char size = *fmts++;

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
            else if (c == 's')
            {
                char * str = va_arg(args, char *);
                
                TERMTRY1(writeString(term, str), res, cnt);
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

TerminalWriteResult TerminalBase::WriteUIntD(const uint64_t val)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    char str[21];
    str[20] = 0;

    size_t i;
    uint64_t x = val;

    for (i = 0; x > 0 && i < 20; ++i)
    {
        uint8_t digit = x % 10;

        str[19 - i] = '0' + digit;

        x /= 10;
    }

    return this->Descriptor->WriteString(this, str + 20 - i);
}

TerminalWriteResult TerminalBase::WriteHex16(const uint16_t val)
{
    if (!this->Descriptor->Capabilities.CanOutput)
        return {Handle(HandleResult::UnsupportedOperation), 0U, InvalidCoordinates};

    char str[5];
    str[4] = '\0';

    for (size_t i = 0; i < 4; ++i)
    {
        uint8_t nib = (val >> (i * 4)) & 0xF;

        str[3 - i] = (nib > 9 ? '7' : '0') + nib;
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
        uint8_t nib = (val >> (i * 4)) & 0xF;

        str[15 - i] = (nib > 9 ? '7' : '0') + nib;
    }

    return this->Descriptor->WriteString(this, str);
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
