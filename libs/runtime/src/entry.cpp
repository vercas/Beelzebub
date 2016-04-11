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

#include <crt0.hpp>
// #include <syscalls.h>
#include <terminals/debug.hpp>
#include <self.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

/// Global constructors.
__extern __used void _init(void);

static DebugTerminal procDbgTrm;

__extern __bland __used void _start(char * args)
{
    // PerformSyscall(SyscallSelection::DebugPrint, const_cast<char *>("\r\nLIBRARY ENTRY POINT\r\n"), 0, 0, 0, 0);

    _init();

    Debug::DebugTerminal = &procDbgTrm;

    // PerformSyscall(SyscallSelection::DebugPrint, const_cast<char *>("\r\nABOUT TO TRY DEBUG_TERM\r\n"), 0, 0, 0, 0);

    DEBUG_TERM  << "Testing stream operator on the debug terminal inside the "
                << "runtime's entry point!" << EndLine;

    DEBUG_TERM  << Self.GetSymbol("Self")       << EndLine
                << Self.GetSymbol("_start")     << EndLine
                << Self.GetSymbol("__start")    << EndLine
                << Self.GetSymbol("_init")      << EndLine
                << Self.GetSymbol("_fini")      << EndLine
                << Self.GetSymbol("BLEEEERGH")  << EndLine;

    QuitProcess(HandleResult::Okay, 0);
}

__extern void __start(char * input) __alias(_start);
//  Just in case there's any voodoo going on.
