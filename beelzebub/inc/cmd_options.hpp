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
#include <string.h>
#include <handles.h>

#define CMDO(name, sf, lf, vt)                      \
CommandLineOptionSpecification MCATS(CMDO_, name)   \
{                                                   \
    sf,                                             \
    lf,                                             \
    CommandLineOptionValueTypes::vt                 \
}

namespace Beelzebub
{
    /**
     *  Possible types of command-line option values.
     */
    enum class CommandLineOptionValueTypes
        : uintptr_t
    {
        BooleanByPresence = 0,
        BooleanExplicit = 1,
        String = 2,
        SignedInteger = 3,
        UnsignedInteger = 4,
        Float = 5,
    };

    /**
     *  Represents specification for parsing command line options.
     */
    struct CommandLineOptionSpecification
    {
    public:
        /*  Constructor(s)  */

        inline CommandLineOptionSpecification(char const * const sf
                                            , char const * const lf
                                            , CommandLineOptionValueTypes const vt)
            : ShortForm(sf)
            , LongForm(lf)
            , ValueType(vt)
            , ParsingResult()
            , StringValue(nullptr)
        {

        }

        /*  Fields  */

        char const * ShortForm;
        char const * LongForm;

        const CommandLineOptionValueTypes ValueType;

        Handle ParsingResult;

        union
        {
            char *   StringValue;
            bool     BooleanValue;
            int64_t  SignedIntegerValue;
            uint64_t UnsignedIntegerValue;
        };
    };

    /**
     *  Represents the state of a command-line option parser.
     */
    class CommandLineOptionParserState
    {
    public:
        /*  Constructor(s)  */

        inline CommandLineOptionParserState()
            : InputString(nullptr)
            , Length(0)
            , TokenCount(0)
            , Done(false)
            , Started(false)
        {
            //  Nuthin'.
        }

        /*  Operations  */

        Handle StartParsing(char * const input);

        Handle ParseOption(CommandLineOptionSpecification & opt);

        /*  Fields  */

        char * InputString;
        size_t Length;
        size_t TokenCount;

        bool Done;
        bool Started;
    };
}
