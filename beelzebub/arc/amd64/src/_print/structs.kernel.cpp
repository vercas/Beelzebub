/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

#include <beel/terminals/base.hpp>
#include <beel/structs.kernel.h>

using namespace Beelzebub;

/*********************
    DEBUG PRINTING
*********************/

namespace Beelzebub { namespace Terminals
{
    template<>
    TerminalBase & operator << <GeneralRegisters64 *>(TerminalBase & term, GeneralRegisters64 * const val)
    {
        term.WriteFormat("General Registers (AMD64) %Xp:%n"
            "\tStack Segment: %X2%n"
            "\tData Segment: %X2%n"
            "\tCode Segment: %X2%n"
            "\tStack Pointer: %Xp%n"
            "\tInstruction Pointer: %Xp%n"
            "\tFlags: %Xs%n"
            "\t----%n"
            "\tRDI: %Xs%n"
            "\tRAX: %Xs%n"
            "\tRCX: %Xs%n"
            "\tRDX: %Xs%n"
            "\tRBP: %Xs%n"
            "\t----%n"
            //"\tVector: %u1%n"
            "\tError Code: %Xs%n"
            , val
            , (uint16_t)val->SS
            , (uint16_t)val->DS
            , (uint16_t)val->CS
            , val->RSP
            , val->RIP
            , val->RFLAGS
            , val->RDI
            , val->RAX
            , val->RCX
            , val->RDX
            , val->RBP
            //, (uint8_t)val->Vector
            , val->ErrorCode);

        return term;
    }
}}
