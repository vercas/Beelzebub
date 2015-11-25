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

#include <execution/thread_init.hpp>
#include <system/cpu.hpp>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;

void Beelzebub::Execution::InitializeThreadState(Thread * const thread)
{
    thread->KernelStackTop &= ~((uintptr_t)0xF);
    thread->KernelStackBottom = (thread->KernelStackBottom + 0xF) & ~((uintptr_t)0xF);
    //  Makin' sure the stack is aligned on a 16-byte boundary.

    thread->KernelStackPointer = thread->KernelStackTop - sizeof(ThreadState);

    ThreadState * initState = (ThreadState *)thread->KernelStackPointer;

    initState->RIP = (uintptr_t)thread->EntryPoint;
    initState->CS = Cpu::GetCs();
    initState->DS = Cpu::GetDs();
    initState->SS = Cpu::GetSs();

    //initState->Vector = 3;  //  Uhm, not sure if the value matters but a breakpoint is the least bad.
    initState->ErrorCode = 0;

    initState->RFLAGS = (uint64_t)(FlagsRegisterFlags::Reserved1 | FlagsRegisterFlags::InterruptEnable | FlagsRegisterFlags::Cpuid);

    initState->RSP = initState->RBP = thread->KernelStackTop;
    //  Upon interrupt return, the stack will be clean.

    initState->RAX = 0;
    initState->RBX = 0;
    initState->RCX = 0;
    initState->RDX = 0;
    initState->RSI = 0;
    initState->RDI = 0;
    initState->R8  = 0;
    initState->R9  = 0;
    initState->R10 = 0;
    initState->R11 = 0;
    initState->R12 = 0;
    initState->R13 = 0;
    initState->R14 = 0;
    initState->R15 = 0;

    thread->State = *initState;
}

Handle Beelzebub::Execution::InitializeBootstrapThread(Thread * const bst, Process * const bsp, MemoryManager * const bsmm)
{
    new (bsp) Process(bsmm);
    new (bst) Thread(bsp);

	uint64_t dummy = 0x0056657263617300;
	//	Just a dummy value.

	bst->KernelStackBottom = 0xFFFFFFFFFFFFC000U;//RoundDown((uintptr_t)&dummy, PageSize);
	bst->KernelStackTop = 0xFFFFFFFFFFFFF000U;//RoundUp((uintptr_t)&dummy, PageSize);

	bst->Next = bst->Previous = bst;

	return HandleResult::Okay;
}
