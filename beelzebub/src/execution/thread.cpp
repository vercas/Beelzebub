#include <execution/thread.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

/******************
Thread class
*******************/

/*  Linkage  */

Handle Thread::IntroduceNext(Thread * const other)
{
    Thread * oldNext = this->Next;

    Thread * current = oldNext;

    do
    {
        if (current == other)
            return HandleResult::ThreadAlreadyLinked;

        current = current->Next;
    }
    while (current != oldNext);

    this->Next = other;
    oldNext->Previous = other;

    other->Previous = this;
    other->Next = oldNext;

    return HandleResult::Okay;
}
