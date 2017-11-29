/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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

#include <beel/terminals/cache.hpp>
#include <string.h>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

/*  Cache terminal descriptor  */

static TerminalCapabilities CacheTerminalCapabilities = {
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

/**************************
    CacheTerminal class
**************************/

/*  Constructors    */

CacheTerminal::CacheTerminal(AcquireCharPoolFunc const acquire
                           , EnlargeCharPoolFunc const enlarge
                           , ReleaseCharPoolFunc const release)
    : TerminalBase( &CacheTerminalCapabilities )
    , AcquirePool(acquire)
    , EnlargePool(enlarge)
    , ReleasePool(release)
    , FirstPool(nullptr)
    , Capacity(0)
    , Count(0)
    , FreeCount(0)
{

}

/*  Writing  */

TerminalWriteResult CacheTerminal::InternalWrite(char const * const str, size_t inLen)
{
    if unlikely(this->AcquirePool == nullptr)
        return {HandleResult::ObjectDisposed, 0U, InvalidCoordinates};
    
    if unlikely(str == nullptr)
        return {HandleResult::ArgumentNull, 0U, InvalidCoordinates};

    size_t const len = Minimum(strlen(str), inLen);
    size_t written = 0, left = len;

    if unlikely(len == 0)
        return {HandleResult::Okay, 0U, InvalidCoordinates};
    //  Very unlikely that an empty string was given, but a lot of useless
    //  computation can be avoided if handled separately.

    CharPool * pool = this->FirstPool, * last = nullptr;

    while (pool != nullptr)
    {
        if (pool->Size < pool->Capacity)
            break;
        //  There is room in this pool.

        last = pool;
        pool = pool->Next;
    }

copyIntoPool:

    if (pool != nullptr)
    {
        size_t const copyCnt = Minimum(pool->Capacity - pool->Size, left);
        char * const dst = reinterpret_cast<char *>(pool) + sizeof(CharPool) + pool->Size;

        ::memcpy(dst, str + written, copyCnt);

        pool->Size += copyCnt;
        written += copyCnt;
        left -= copyCnt;
        this->Count += copyCnt;
        this->FreeCount -= copyCnt;

        last = pool;
    }

    if (left > 0)
    {
        if (pool != nullptr && pool->Next != nullptr)
        {
            //  There appeared to be a next pool...

            pool = pool->Next;

            goto copyIntoPool;
        }

        //  Okay, so there are characters left to pool but there is no pool
        //  available. Thus, the first attempt is at enlarging the last pool.

        Handle res;

        if (last != nullptr)
        {
            uint32_t const currentCapacity = last->Capacity;

            res = this->EnlargePool(left, sizeof(CharPool), last);

            if unlikely(res.IsOkayResult())
            {
                //  It's not really unlikely to succeed, but this branch will go
                //  back, so it's best to move it out of the way, if possible.

                uint32_t const capacityDiff = last->Capacity - currentCapacity;

                this->Capacity += capacityDiff;
                this->FreeCount += capacityDiff;

                pool = last;

                goto copyIntoPool;
                //  Start/resume copying.
            }
        }

        //  Perhaps enlarging failed, or there is no previous pool to enlarge.
        //  In this case, a new pool is acquired...

        res = this->AcquirePool(left, sizeof(CharPool), pool);

        if unlikely(!res.IsOkayResult())
            return {res, (uint32_t)written, InvalidCoordinates};
        //  Could not acquire a new pool, so the characters that are left cannot
        //  be written anywhere.

        //  At this point, creating the new pool succeeded.

        if (last != nullptr)
            last->Next = pool;
        else
            this->FirstPool = pool;
        //  Link it up.

        goto copyIntoPool;
        //  Start/resume copying.
    }

    return {HandleResult::Okay, (uint32_t)written, InvalidCoordinates};
}

TerminalWriteResult CacheTerminal::WriteUtf8(const char * c)
{
    size_t i = 1;
    char temp[7] = "\0\0\0\0\0\0";
    temp[0] = *c;
    //  6 bytes + null terminator in UTF-8 character.

    if ((*c & 0xC0) == 0xC0)
        do
        {
            temp[i] = c[i];

            ++i;
        } while ((c[i] & 0xC0) == 0x80 && i < 7);
        //  This copies the remainder of the bytes, up to 6.

    return this->InternalWrite(temp, i);
}

TerminalWriteResult CacheTerminal::Write(const char * const str, size_t len)
{
    return this->InternalWrite(str, len);
}

/*  Flushing & Cleanup  */

TerminalWriteResult CacheTerminal::Dump(TerminalBase & target) const
{
    if unlikely(this->AcquirePool == nullptr)
        return {HandleResult::ObjectDisposed, 0U, InvalidCoordinates};
    
    TerminalWriteResult res {};
    uint32_t cnt;

    CharPool * pool = this->FirstPool;

    while (pool != nullptr)
    {
        TERMTRY1(target.Write(pool->GetString()), res, cnt);

        pool = pool->Next;
    }

    return res;
}

void CacheTerminal::Clear()
{
    if unlikely(this->AcquirePool == nullptr)
        return;
    
    CharPool * pool = this->FirstPool;

    while (pool != nullptr)
    {
        ::memset(const_cast<char *>(pool->GetString()), 0, pool->Size);
        pool->Size = 0;

        pool = pool->Next;
    }
}

static Handle ReleasePoolList(CacheTerminal & term, CharPool * & pool)
{
    if unlikely(pool == nullptr)
        return HandleResult::Okay;

    Handle res = ReleasePoolList(term, pool->Next);
    //  Yes, the last pool is released first.

    if unlikely(!res.IsOkayResult())
        return res;

    res = term.ReleasePool(sizeof(CharPool), pool);

    if likely(res.IsOkayResult())
        pool = nullptr;

    return res;
}

Handle CacheTerminal::Destroy()
{
    if unlikely(this->AcquirePool == nullptr)
        return HandleResult::ObjectDisposed;

    return ReleasePoolList(*this, this->FirstPool);
}

/*  Now to implement the << operator.  */

namespace Beelzebub { namespace Terminals
{
    template<>
    TerminalBase & operator << <CacheTerminal>(TerminalBase & term, CacheTerminal const value)
    {
        value.Dump(term);

        return term;
    }
}}
