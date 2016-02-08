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
*/

#include <cmd_options.hpp>
#include <debug.hpp>

using namespace Beelzebub;

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
    this->Length = len = strlen(input);

    bool underEscape = false, underQuotes = false;

    for (size_t i = 0; i < len; ++i)
    {
        if (underEscape)
        {
            //  Maybe do something here..?

            underEscape = false;
        }
        else if (underQuotes)
        {
            switch (input[i])
            {
                case '\\':
                    underEscape = true;
                    break;

                case '"':
                    underQuotes = false;
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

                case '"':
                    underQuotes = true;
                    break;
            }
        }
    }

    this->Started = true;

    msg("Length of \"%s\" is: %us.%n", this->InputString, this->Length);

    return HandleResult::Okay;
}

Handle CommandLineOptionParserState::ParseOption(CommandLineOptionSpecification & opt)
{


    return HandleResult::Okay;
}
