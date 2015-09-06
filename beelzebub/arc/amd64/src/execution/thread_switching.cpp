#include <execution/thread_switching.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

void Beelzebub::Execution::SwitchNext(Thread * const current)
{
    SwitchThread(&current->KernelStackPointer, current->Next->KernelStackPointer);
}

bool Beelzebub::Execution::SetNext(Thread * const current, Thread * const next)
{
	Thread* n = current;
	while (n->Next != current) {
		if (n == next)
			return false;
	}

	n = current->Next;
	current->Next = next;
	n->Previous = next;
	next->Next = n;
	next->Previous = current;
	return true;
}