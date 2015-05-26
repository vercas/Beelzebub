#pragma once

#define __BEELZEBUB__DEBUG  true

#include <architecture.h>
#include <metaprogramming.h>

namespace Beelzebub
{
    extern TerminalBase * MainTerminal;

    __cold __bland void Main();
    __cold __bland void Secondary();
}
