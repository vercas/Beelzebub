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

#include <global_options.hpp>

using namespace Beelzebub;

CommandLineOptionSpecification Beelzebub::CMDO_Term;
CommandLineOptionSpecification Beelzebub::CMDO_SmpEnable;

CommandLineOptionSpecification * Beelzebub::CommandLineOptionsHead;

Handle Beelzebub::InstanceGlobalOptions()
{
    CMDO_EX(Term, "t", "term", String);
    CMDO_LINKED_EX(SmpEnable, nullptr, "smp", BooleanExplicit, Term);

    CommandLineOptionsHead = &CMDO_SmpEnable;

    return HandleResult::Okay;
}

// CMDO_LINKED(02, "b", "bbb", String, 01);
// CMDO_LINKED(03, "c", "ccc", String, 02);
// CMDO_LINKED(04, "d", "ddd", BooleanByPresence, 03);
// CMDO_LINKED(05, "e", "eee", BooleanByPresence, 04);
// CMDO_LINKED(06, "freud", "booohoooo", BooleanByPresence, 05);
// CMDO_LINKED(07, "g", "ggg", BooleanByPresence, 06);
