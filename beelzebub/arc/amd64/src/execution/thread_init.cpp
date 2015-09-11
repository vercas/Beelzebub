#include <execution/thread_init.hpp>
#include <system\cpu.hpp>

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

    initState->Vector = 3;  //  Uhm, not sure if the value matters but a breakpoint is the least bad.
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

}

Handle Beelzebub::Execution::InitializeBootstrapThread(Thread * const bst, Process * const bsp, MemoryManager * const bsmm)
{
    //new (bst) Thread();

	uint64_t dummy = 0x0056657263617300;
	//	Just a dummy value.

	bst->KernelStackBottom = (uintptr_t)&dummy & ~(uintptr_t)0xFFF;
	bst->KernelStackTop = ((uintptr_t)&dummy + 0xFFF) & ~(uintptr_t)0xFFF;

	bst->Next = bst->Previous = bst;

    //new (bsp) Process();

    bsp->VAS = bsmm;

    bst->Owner = bsp;

	return Handle(HandleResult::Okay);
}
