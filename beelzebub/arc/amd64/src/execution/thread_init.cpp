#include <execution/thread_init.hpp>
//#include <execution/thread.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

void Beelzebub::Execution::InitializeThreadState(Thread * const thread)
{
    thread->KernelStackTop &= ~((uintptr_t)0xF);
    thread->KernelStackBottom = (thread->KernelStackBottom + 0xF) & ~((uintptr_t)0xF);
    //  Makin' sure the stack is aligned on a 16-byte boundary.

    thread->KernelStackPointer = thread->KernelStackTop - sizeof(ThreadState);

    ThreadState * initState = (ThreadState *)thread->KernelStackPointer;
    initState->RIP = (uintptr_t)thread->EntryPoint;
}

Handle Beelzebub::Execution::InitializeBootstrapThread(Thread * const bst)
{
	uint64_t dummy = 0x0056657263617300;
	//	Just a dummy value.

	bst->KernelStackBottom = (uintptr_t)&dummy & ~(uintptr_t)0xFFF;
	bst->KernelStackTop = ((uintptr_t)&dummy + 0xFFF) & ~(uintptr_t)0xFFF;

	bst->Next = bst->Previous = bst;

	return Handle(HandleResult::Okay);
}

Handle Beelzebub::Execution::SpawnThread(Thread * const thread, ThreadEntryPointFunction func)
{
	// TODO: Use heap ASAP
	static uint64_t dummy = 0x0036656263617300;
	thread->KernelStackBottom = (uintptr_t)&dummy & ~(uintptr_t)0xFFF;
	thread->KernelStackTop = ((uintptr_t)&dummy + 0xFFF) & ~(uintptr_t)0xFFF;
	dummy -= 0x1000;

	thread->EntryPoint = func;
	InitializeThreadState(thread);
	return Handle(HandleResult::Okay);
}