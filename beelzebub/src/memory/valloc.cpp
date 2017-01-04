/*
    Copyright (c) 2017 Alexandru-Mihai Maftei. All rights reserved.


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

#ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_VALLOC

#include <valloc/platform.hpp>
#include "memory/vmm.hpp"
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::Memory;
using namespace Valloc;

void Platform::AllocateMemory(void * & addr, size_t & size)
{
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(addr);

    Handle res = Vmm::AllocatePages(nullptr, size
        , MemoryAllocationOptions::VirtualKernelHeap | MemoryAllocationOptions::AllocateOnDemand
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::Generic
        , vaddr);

    // MSG_("Allocating memory for vAlloc: %Xp %Xs %H%n", vaddr, size, res);

    if unlikely(res != HandleResult::Okay)
    {
        addr = nullptr;
        size = 0;
    }
    else
    {
        addr = reinterpret_cast<void *>(vaddr);
    }
}

void Platform::FreeMemory(void * addr, size_t size)
{
    // MSG_("Freeing memory for vAlloc: %Xp %Xs%n", addr, size);

    Vmm::FreePages(nullptr, reinterpret_cast<uintptr_t>(addr), size);
}

void Platform::ErrorMessage(char const * fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    if likely(DebugTerminal != nullptr)
        withLock (MsgSpinlock)
        {
            DebugTerminal->Write(fmt, args);
            DebugTerminal->WriteLine();
        }

    va_end(args);
}

void Platform::Abort(char const * file, size_t line, char const * cond, char const * fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    CatchFireV(file, line, cond, fmt, args);

    va_end(args);
}

#endif
