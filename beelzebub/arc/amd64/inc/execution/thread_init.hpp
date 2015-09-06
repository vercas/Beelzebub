#pragma once

#include <execution/thread.hpp>
#include <execution/thread_state.hpp>
#include <handles.h>
#include <metaprogramming.h>

namespace Beelzebub { namespace Execution
{
    __bland void InitializeThreadState(Thread * const thread);

    __bland __cold Handle InitializeBootstrapThread(Thread * const bst);

    __bland Handle SpawnThread(Thread * const thread, ThreadEntryPointFunction func);
}}
