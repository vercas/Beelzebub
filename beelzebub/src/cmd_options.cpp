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

/*
    The input string must be prepared a little before it can be used efficiently.
    This is done with a two-pass algorithm.

    The first pass identifies the different elements of the input. An element is
    a component of an option. Either its name, its value or both.

    The second pass sanitizes the individual tokens so they can be used more
    quickly and efficiently by the individual option parsing function.
    Sanitation means expanding escape sequences and compacting the elements
    (removing multiplicate null terminators).
*/

#include <cmd_options.hpp>
#include <utils/conversions.hpp>
#include <string.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

/*****************************************
    CommandLineOptionParserState class
*****************************************/

/*  Operations  */

Handle CommandLineOptionParserState::StartParsing(char * const input)
{
    if (this->Started)
        return HandleResult::UnsupportedOperation;

    size_t len;

    this->InputString = input;
    len = strlen(input);

    //  First pass.

    bool underEscape = false, underSingleQuotes = false, underDoubleQuotes = false;

    for (size_t i = 0; i < len; ++i)
    {
        if (underEscape)
        {
            //  Nothing to do here at this step.

            underEscape = false;
        }
        else if (underSingleQuotes)
        {
            switch (input[i])
            {
                case '\\':
                    underEscape = true;
                    break;

                case '\'':
                    underSingleQuotes = false;
                    break;
            }
        }
        else if (underDoubleQuotes)
        {
            switch (input[i])
            {
                case '\\':
                    underEscape = true;
                    break;

                case '"':
                    underDoubleQuotes = false;
                    break;
            }
        }
        else
        {
            switch (input[i])
            {
                case '\\':
                    underEscape = true;
                    break;

                case '\'':
                    underSingleQuotes = true;
                    break;

                case '"':
                    underDoubleQuotes = true;
                    break;

                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    input[i] = '\0';
                    break;
            }
        }
    }

    if (underEscape || underSingleQuotes || underDoubleQuotes)
        return HandleResult::CmdOptionsMalformatted;

    //  Second pass.

    size_t j = 0, tkCnt = 0;
    bool lastWasNull = true;

    for (size_t i = 0; i < len; ++i)
    {
        if (underEscape)
        {
            underEscape = false;

            switch (input[i])
            {
                case '0':
                    input[j++] = '\0';
                    break;

                case 'a':
                    input[j++] = '\a';
                    break;

                case 'b':
                    input[j++] = '\b';
                    break;

                case 'f':
                    input[j++] = '\f';
                    break;

                case 'n':
                    input[j++] = '\n';
                    break;

                case 'r':
                    input[j++] = '\r';
                    break;

                case 'v':
                    input[j++] = '\v';
                    break;

                case 't':
                    input[j++] = '\t';
                    break;

                default:
                    input[j++] = input[i];
                    break;
            }
        }
        else if (underSingleQuotes)
        {
            switch (input[i])
            {
                case '\\':
                    underEscape = true;
                    break;

                case '\'':
                    underSingleQuotes = false;
                    break;

                default:
                    input[j++] = input[i];
                    break;
            }
        }
        else if (underDoubleQuotes)
        {
            switch (input[i])
            {
                case '\\':
                    underEscape = true;
                    break;

                case '"':
                    underDoubleQuotes = false;
                    break;

                default:
                    input[j++] = input[i];
                    break;
            }
        }
        else
        {
            if (input[i] == '\0')
            {
                if (!lastWasNull)
                {
                    lastWasNull = true;

                    input[j++] = input[i];

                    ++tkCnt;
                }

                //  Else do nuthin'.
            }
            else
            {
                lastWasNull = false;

                switch (input[i])
                {
                    case '\\':
                        underEscape = true;
                        break;

                    case '\'':
                        underSingleQuotes = true;
                        break;

                    case '"':
                        underDoubleQuotes = true;
                        break;

                    default:
                        input[j++] = input[i];
                        break;
                }
            }
        }
    }

    if (!lastWasNull)
    {
        input[j] = '\0';

        ++tkCnt;
    }
    else
        --j;

    this->Length = j;
    this->TokenCount = tkCnt;

    //  Done.

    this->Started = true;

    return HandleResult::Okay;
}

__noinline Handle HandleValue(CommandLineOptionSpecification & opt, char * input);
//  Forward declaration.

Handle CommandLineOptionParserState::ParseOption(CommandLineOptionSpecification & opt)
{
    if (opt.ParsingResult.IsOkayResult())
        return opt.ParsingResult;

    // msg("(( Trying option \"%s\"/\"%s\" ))%n", opt.ShortForm, opt.LongForm);

    for (size_t i = 0, len; i + 1 < this->Length; i += len + 1)
    {
        len = strlen(this->InputString + i);
        //  The incrementation step will move 'i' to the next token, basically.

        if (this->InputString[i] != '-')
            continue;
        //  Not even an option.

        // msg("<< TRYING @ %us, len %us; \"%s\" >>%n", i, len, this->InputString + i);

        if (opt.ShortForm != nullptr && this->InputString[i + 1] != '-')
        {
            if (opt.ValueType != CommandLineOptionValueTypes::BooleanByPresence
             && i + 2 + len >= this->Length)
                break;
            //  Definitely not this one.

            if (opt.ShortForm[0] != '\0' && opt.ShortForm[1] == '\0')
            {
                //  One-letter short option means more can be grouped together.

                if (memchr(this->InputString + i + 1, opt.ShortForm[0], len - 1) == nullptr)
                    continue;
                //  Flag not found.
            }
            else if (strncmp(this->InputString + i + 1, opt.ShortForm, len) != 0)
                continue;
            //  If the short form has more than one character, it must match the
            //  whole option. Null terminator included.

            //  So, if a short form is defined and the current token is '-'
            //  followed by the short form, this is a hit!

            // msg("!!HIT!!%n");

            return opt.ParsingResult = HandleValue(opt, this->InputString + i + 1 + len);
        }
        else if (opt.LongForm != nullptr && this->InputString[i + 1] == '-')
        {
            len = strlen(opt.LongForm);
            //  Repurposed!

            if (opt.ValueType != CommandLineOptionValueTypes::BooleanByPresence
             && i + 2 + len >= this->Length)
                break;
            //  Not enough room.

            if (strncmp(this->InputString + i + 2, opt.LongForm, len) != 0)
                continue;
            //  No match.

            if (this->InputString[i + 2 + len] != '\0' && this->InputString[i + 2 + len] != '=')
                continue;
            //  This is just a substring, oddly.

            //  So, if a long form is defined and the current token is "--"
            //  followed by the long form, this is a hit!

            // msg("!!HIT!!%n");

            return opt.ParsingResult = HandleValue(opt, this->InputString + i + 3 + len);
            //  Handles both '=' and a null character.
        }
    }

    if (opt.ValueType == CommandLineOptionValueTypes::BooleanByPresence)
    {
        opt.BooleanValue = false;

        return opt.ParsingResult = HandleResult::Okay;
    }

    return opt.ParsingResult = HandleResult::Okay;  //  TODO: NOT GOOD!
}

/********************************************
    Utilitary Methods for Parsing Options
********************************************/

Handle HandleValue(CommandLineOptionSpecification & opt, char * input)
{
    switch (opt.ValueType)
    {
        case CommandLineOptionValueTypes::BooleanByPresence:
            opt.BooleanValue = true;

            return HandleResult::Okay;

        case CommandLineOptionValueTypes::BooleanExplicit:
            return FromString<bool>(input, opt.BooleanValue);

        case CommandLineOptionValueTypes::String:
            opt.StringValue = input;

            return HandleResult::Okay;

        case CommandLineOptionValueTypes::SignedInteger:
            return FromString<int64_t>(input, opt.SignedIntegerValue);

        case CommandLineOptionValueTypes::UnsignedInteger:
            return FromString<uint64_t>(input, opt.UnsignedIntegerValue);

        case CommandLineOptionValueTypes::Float:
            return HandleResult::CmdOptionValueTypeInvalid;
            //  TODO: Implement this...
    }

    return HandleResult::CmdOptionValueTypeInvalid;
}
