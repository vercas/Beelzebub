#include <execution/thread_switching.hpp>
#include <handles.h>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

static Thread* CurrentThread;

void Beelzebub::Execution::SwitchNext(Thread * const current)
{
	CurrentThread = current->Next;
    SwitchThread(&current->KernelStackPointer, current->Next->KernelStackPointer);
}

Handle Beelzebub::Execution::SetNext(Thread * const current, Thread * const next)
{
	Thread* n = current;
	while (n->Next != current) {
		if (n == next)
			return Handle(HandleResult::UnsupportedOperation);
	}

	n = current->Next;
	current->Next = next;
	n->Previous = next;
	next->Next = n;
	next->Previous = current;
	return Handle(HandleResult::Okay);
}

Handle Beelzebub::Execution::DestroyThread(Thread * const thread)
{
	thread->Previous->Next = thread->Next;
	thread->Next->Previous = thread->Previous;
	if (thread == CurrentThread)
		SwitchNext(thread);
	return Handle(HandleResult::Okay);
}

Thread* GetCurrentThread()
{
	return CurrentThread;
}