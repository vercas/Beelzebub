#include <execution/thread_switching.hpp>
#include <system/cpu.hpp>
#include <handles.h>
#include <kernel.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Execution;

static Thread* CurrentThread = nullptr;

void Beelzebub::Execution::SwitchNext()
{
	SwitchTo(GetCurrentThread()->Next);
}

void Beelzebub::Execution::SwitchTo(Thread * const thread)
{
	if (CurrentThread != nullptr)
		CurrentThread->Executing = false;
	thread->Executing = true;
    CurrentThread = thread;
    MainTerminal->Write("Switching to ");
	MainTerminal->WriteUIntD(thread->ThreadID);
	MainTerminal->WriteLine();
    SwitchThread(&CurrentThread->KernelStackPointer, &thread->KernelStackPointer);
}

Handle Beelzebub::Execution::SetNext(Thread * const current, Thread * const next)
{
	Thread* n = current;
	while (n->Next != current) {
		if (n == next)
			return Handle(HandleResult::UnsupportedOperation);
		n = n->Next;
	}

	next->Next = current->Next;
	next->Previous = current;
	current->Next = next;
	next->Next->Previous = next;
	return Handle(HandleResult::Okay);
}

Handle Beelzebub::Execution::DestroyThread(Thread * const thread)
{
	MainTerminal->Write("Destroying ");
	MainTerminal->WriteUIntD(thread->ThreadID);
	MainTerminal->WriteLine();

	thread->Previous->Next = thread->Next;
	thread->Next->Previous = thread->Previous;
	thread->Completed = true;
	if (thread == GetCurrentThread()) {
		SwitchNext();
		// TODO: Throw exception here
	}
	return Handle(HandleResult::Okay);
}

Thread* Beelzebub::Execution::GetCurrentThread()
{
	return CurrentThread;
}