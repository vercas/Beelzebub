#include <execution/thread_init.hpp>

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
