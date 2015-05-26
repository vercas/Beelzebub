#pragma once

#include <terminals/base.hpp>
#include <metaprogramming.h>

using namespace Beelzebub::Terminals;

__extern __bland void InitializeMemory();
__extern __bland void InitializeInterrupts();

__extern __bland TerminalBase * InitializeTerminalProto();
__extern __bland TerminalBase * InitializeTerminalMain();
