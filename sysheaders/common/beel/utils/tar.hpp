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

#pragma once

#include <beel/metaprogramming.h>

namespace Beelzebub { namespace Utils
{
    #define ENUM_TARHEADERTYPE(ENUMINST)    \
        ENUMINST(None               , '\0') \
        ENUMINST(File               , '0' ) \
        ENUMINST(Link               , '1' ) \
        ENUMINST(Reserved1          , '2' ) \
        ENUMINST(CharacterSpecial   , '3' ) \
        ENUMINST(BlockSpecial       , '4' ) \
        ENUMINST(Directory          , '5' ) \
        ENUMINST(FifoSpecial        , '6' ) \
        ENUMINST(Reserved2          , '7' )

    __ENUMDECL(TarHeaderType, ENUM_TARHEADERTYPE, LITE, char)
    __ENUM_TO_STRING_DECL(TarHeaderType, ENUM_TARHEADERTYPE);

    static_assert(sizeof(TarHeaderType) == 1, "");

    struct TarHeader
    {
        /*  Statics  */

        static size_t GetNumber(char const * str, size_t size);

        /*  Fields  */

        char Name[100];               /*   0 -  99 */
        char Mode[8];                 /* 100 - 107 */
        char UID[8];                  /* 108 - 115 */
        char GID[8];                  /* 116 - 123 */
        char Size[12];                /* 124 - 135 */
        char MTime[12];               /* 136 - 147 */
        char Checksum[8];             /* 148 - 155 */
        TarHeaderType TypeFlag;       /* 156       */
        char LinkName[100];           /* 157 - 256 */
        char Magic[6];                /* 257 - 262 */
        char Version[2];              /* 263 - 264 */
        char UName[32];               /* 265 - 296 */
        char GName[32];               /* 297 - 328 */
        char DevMajor[8];             /* 329 - 336 */
        char DevMinor[8];             /* 337 - 344 */
        char Prefix[155];             /* 345 - 499 */
        char Padding[12];             /* 500 - 511 */

        /*  Properties  */

        inline size_t GetUid()      const { return GetNumber(this->UID     ,  7); }
        inline size_t GetGid()      const { return GetNumber(this->GID     ,  7); }
        inline size_t GetSize()     const { return GetNumber(this->Size    , 11); }
        inline size_t GetChecksum() const { return GetNumber(this->Checksum,  7); }

        bool IsZero() const;

        inline bool IsFile()      const { return this->TypeFlag == TarHeaderType::File;      }
        inline bool IsDirectory() const { return this->TypeFlag == TarHeaderType::Directory; }

        /*  Iteration  */

        TarHeader * GetNext() const;

        /*  Query  */

        comp_t CompareName(char const * str) const;
        bool IsInDirectory(char const * dir, bool includeSelf = false) const;
    };

    static_assert(sizeof(TarHeader) == 512, "");
}}
