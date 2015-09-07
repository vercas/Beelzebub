#pragma once

#include <execution/thread.hpp>
#include <metaprogramming.h>
#include <handles.h>

__extern __hot void SwitchThread(uintptr_t * curStack, uintptr_t nextStack);

namespace Beelzebub { namespace Execution
{
    __hot __bland void SwitchNext(Thread * const current);

    __hot __bland Handle SetNext(Thread * const current, Thread * const next);

    __hot __bland Handle DestroyThread(Thread * const thread);

    __hot __bland Thread* GetCurrentThread();
}}
