#pragma once

#include <execution/thread_state.hpp>

namespace Beelzebub { namespace Execution
{
	class Thread
	{
	public:

		uintptr_t StackPointer;

		Thread * Next;
	};
}}
