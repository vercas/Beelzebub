#include <execution/thread_switching.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

void Beelzebub::Execution::ScheduleNext(Thread * const current)
{
	SwitchThread(&current->StackPointer, current->Next->StackPointer);
}
