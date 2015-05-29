#pragma once

#include <execution/thread.hpp>
#include <metaprogramming.h>

__extern void SwitchThread(uintptr_t * curStack, uintptr_t nextStack);

namespace Beelzebub { namespace Execution
{
	__bland void ScheduleNext(Thread * const current);
}}
