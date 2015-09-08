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

    if (other == nullptr)
        return HandleResult::ArgumentNull;

    if (this != other)
    {
        if (this->VAS != other->VAS)
        {
            res = this->VAS->Switch(other->VAS);

            if (!res.IsOkayResult())
                return res;
        }
    }
    else
        return HandleResult::ArgumentOutOfRange;

    return HandleResult::Okay;
}
