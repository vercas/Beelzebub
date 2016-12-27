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

#include "memory/vas.hpp"
#include <beel/interrupt.state.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;

/*********************
    DEBUG PRINTING
*********************/

namespace Beelzebub { namespace Terminals
{
    template<>
    TerminalBase & operator << <Vas *>(TerminalBase & term, Vas * const vas)
    {
        term.WriteLine(" Region Pointer |     Start      |      End       |      Size      |flgs|prt|Content");

        withInterrupts (false)
        {
            vas->Lock.AcquireAsReader();

            MemoryRegion const * reg = vas->First;

            while (reg != nullptr)
            {
                term.WriteFormat("%X8 ", reg);

                term << reg;

                MemoryRegion const * const next = reg->Next;

                if (next != nullptr)
                {
                    assert(next->Prev == reg)
                        ("cur", (void *)reg)
                        ("next", (void *)next)
                        ("next->prev", (void *)next->Prev);
                }

                reg = next;
            }

            vas->Lock.ReleaseAsReader();
        }

        return term;
    }
}}
