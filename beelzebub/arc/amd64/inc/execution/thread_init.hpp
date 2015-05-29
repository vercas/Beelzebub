#pragma once

#include <execution/thread.hpp>
#include <execution/thread_state.hpp>
#include <metaprogramming.h>

namespace Beelzebub { namespace Execution
{
    __bland void InitializeThreadState(Thread * const thread);
}}
