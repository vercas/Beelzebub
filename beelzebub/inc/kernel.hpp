#pragma once

#define __BEELZEBUB__DEBUG  true

#include <architecture.h>
#include <metaprogramming.h>

namespace Beelzebub
{
    extern TerminalBase * MainTerminal;

    __bland void Main();
    __bland void Secondary();
}
