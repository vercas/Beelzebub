#pragma once

#include <execution/thread.hpp>
#include <metaprogramming.h>

__extern __hot void SwitchThread(uintptr_t * curStack, uintptr_t nextStack);

/*namespace Beelzebub { namespace Execution
{
    
}}//*/
