#include <execution/thread_init.hpp>
#include <execution/thread_switching.hpp>
#include <debug.hpp>

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
	bst->Completed = false;
	bst->ThreadID = 0;
	SwitchTo(bst);
	return Handle(HandleResult::Okay);
}

Handle Beelzebub::Execution::SpawnThread(Thread * const thread, ThreadEntryPointFunction func)
{
	static uint64_t FreeTID = 1;
	Handle res;

	// TODO: Use another way to allocate the space asap
	static uint64_t dummy = 0x0046656263617000;
	thread->KernelStackBottom = (uintptr_t)&dummy & ~(uintptr_t)0xFFF;
	thread->KernelStackTop = ((uintptr_t)&dummy + 0xFFF) & ~(uintptr_t)0xFFF;
	dummy -= 0x1000;

	thread->Next = thread->Previous = thread;
	thread->EntryPoint = func;
	thread->Completed = false;
	thread->Executing = false;
	thread->ThreadID = FreeTID++;
	InitializeThreadState(thread);

	res = SetNext(GetCurrentThread(), thread);
	if (!res.IsOkayResult())
	{
		assert(false, "Failed to set next thread: %H", res);
	}

	return Handle(HandleResult::Okay);
}