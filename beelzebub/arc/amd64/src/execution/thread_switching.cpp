#include <execution/thread_switching.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

void Beelzebub::Execution::SwitchNext(Thread * const current)
{
    SwitchThread(&current->KernelStackPointer, current->Next->KernelStackPointer);
}
