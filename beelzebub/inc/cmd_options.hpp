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
     *  Flags indicating options for parsing command-line options.
     */
    enum class CommandLineOptionFlags
        : uintptr_t
    {
        TableFill = 0x0000,
        ReturnFromFunction = 0x0001,
        FlexibleValues = 0x0000,
        LimitedValues = 0x0002,
    };

    ENUMOPS(CommandLineOptionFlags, uintptr_t);

    /**
     *  Represents specification for parsing command line options.
     */
    struct CommandLineOptionSpecification
    {
    public:

        const char * ShortForm;
        const char * LongForm;

        const void * ValueList;
        const size_t ValuesCount;
        const CommandLineOptionValueTypes ValueType;

        const CommandLineOptionFlags Flags;

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

        char * const InputString;
        size_t Length;
        size_t Offset;

        bool Done;
        bool Started;

        __bland __forceinline CommandLineOptionParserState(char * const input)
            : InputString(input)
            , Length(0)
            , Offset(0)
            , Done(false)
            , Started(false)
        {
            //  Nuthin'.
        }
    };

    __bland Handle ParseCommandLineOptions(CommandLineOptionParserState & state);
}
