#include <execution/thread.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

/********************
    Process class
*********************/

/*  Operations  */

Handle Process::SwitchTo(Process * const other)
{
    Handle res;

    if unlikely(other == nullptr)
        return HandleResult::ArgumentNull;

    if likely(this != other)
    {
        if likely(this->Memory != other->Memory)
        {
            res = this->Memory->Switch(other->Memory);

            if unlikely(!res.IsOkayResult())
                return res;
        }
    }
    else
        return HandleResult::ArgumentOutOfRange;

    return HandleResult::Okay;
}
