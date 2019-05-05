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

#include <system/domain.hpp>    //  Platform-specific.
#include <execution/thread.hpp>
#include <beel/terminals/base.hpp>

namespace Beelzebub
{
    enum class MainTerminalInterfaces
    {
        None = 0,

        COM1, COM2, COM3, COM4,
        VBE,
    };

    extern Terminals::TerminalBase * MainTerminal;
    extern MainTerminalInterfaces MainTerminalInterface;

    extern bool Scheduling;
    extern bool CpuDataSetUp;

    extern System::Domain Domain0;

    /**
     *  <summary>Entry point for the bootstrap processor.</summary>
     */
    __startup void Main();

#if   defined(__BEELZEBUB_SETTINGS_SMP)
    /**
     *  <summary>Entry point for application processors.</summary>
     */
    __startup void Secondary();

    /**
     *  <summary>Entry point for other domains.</summary>
     */
    __startup void Ternary();
#endif
}
