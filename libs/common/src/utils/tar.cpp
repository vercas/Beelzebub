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

#include <utils/tar.hpp>
#include <string.h>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

/*  Enums  */

ENUM_TO_STRING_EX2(TarHeaderType, ENUM_TARHEADERTYPE, Beelzebub::Utils)

/***********************
    TarHeader struct
***********************/

/*  Statics  */

size_t TarHeader::GetNumber(char const * str, size_t size)
{
    size_t val = 0;

    switch (size)
    {
    #define CASE(num) case (num): val += (size_t)(str[size - num] - '0') << (3 * (num - 1))

        CASE(12);
        CASE(11);
        CASE(10);
        CASE( 9);
        CASE( 8);
        CASE( 7);
        CASE( 6);
        CASE( 5);
        CASE( 4);
        CASE( 3);
        CASE( 2);
        CASE( 1);

    #undef CASE

        case 0: return val;

        default: return SIZE_MAX;
    }
}

/*  Properties  */

bool TarHeader::IsZero() const
{
    return this->Size[0]  == '\0'
        || this->UID[0]   == '\0'
        || this->GID[0]   == '\0'
        || this->Mode[0]  == '\0'
        || this->TypeFlag == TarHeaderType::None;
}

/*  Iteration  */

TarHeader * TarHeader::GetNext() const
{
    return this->IsZero()
        ? nullptr
        : const_cast<TarHeader *>(
            this + (this->GetSize() + 2 * sizeof(*this) - 1) / sizeof(*this)
        );
}

// TarHeader * TarHeader::GetNext() const
// {
//     TarHeader const * res = this + (this->GetSize() + 2 * sizeof(*this) - 1) / sizeof(*this);

//     return res->IsZero() ? nullptr : const_cast<TarHeader *>(res);
// }

/*  Query  */

comp_t TarHeader::CompareName(char const * str) const
{
    char const * myName = this->Name;

    if (myName[0] == '.')
        myName++;
    if (myName[0] == '/')
        myName++;

    if (str[0] == '.')
        str++;
    if (str[0] == '/')
        str++;

    //  So the leading directory is... NORMALIZED.

    return strncmp(myName, str, 99);
}

bool TarHeader::IsInDirectory(char const * dir, bool includeSelf) const
{
    char const * myName = this->Name;

    if (myName[0] == '.')
        myName++;
    if (myName[0] == '/')
        myName++;

    if (dir[0] == '.')
        dir++;
    if (dir[0] == '/')
        dir++;

    size_t const dirlen = strnlen(dir, 99);

    bool startsEqual = memeq(myName, dir, dirlen);

    return dir[dirlen - 1] == '/' ? startsEqual : (startsEqual && *(myName++ + dirlen) == '/')
        && (includeSelf || myName[dirlen] != '\0');
    //  If the given directory ends in a separator, it was cool. Otherwise, the
    //  comparison may've simply been true because the directory is a substring
    //  of the file name. Therefore, on a missing separator, the next character
    //  in the file name must be that separator. Also, if self isn't included,
    //  the last char of the name must be non-zero.
}

#include <beel/terminals/base.hpp>

/*  Now to implement some << operator magic.  */

namespace Beelzebub { namespace Terminals
{
    #define SPAWN_ENUM(eName) \
    template<> \
    TerminalBase & operator << <eName>(TerminalBase & term, eName const value) \
    { \
        return term << (__underlying_type(eName))(value) << " (" << EnumToString(value) << ")"; \
    }

    SPAWN_ENUM(TarHeaderType)

    template<>
    TerminalBase & operator << <TarHeader const *>(TerminalBase & term, TarHeader const * const value)
    {
        return term << "Tar Header" << EndLine
            << "\tName: " << value->Name << EndLine
            << "\tSize: " << value->Size << " (" << value->GetSize() << ")" << EndLine
            << "\tUID: "  << value->UID  << " (" << value->GetUid()  << ")" << EndLine
            << "\tGID: "  << value->GID  << " (" << value->GetGid()  << ")" << EndLine
            << "\tMode: " << value->Mode << EndLine
            << "\tType Flag: " << value->TypeFlag << EndLine;
    }

    template<>
    TerminalBase & operator << <TarHeader *>(TerminalBase & term, TarHeader * const value)
    {
        return term << const_cast<TarHeader *>(value);
    }

    template<>
    TerminalBase & operator << <TarHeader>(TerminalBase & term, TarHeader const value)
    {
        return term << &value;
    }
}}
