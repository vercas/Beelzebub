#pragma once

#include <metaprogramming.h>

#ifdef __cplusplus
    #include <terminals/base.hpp>

    using namespace Beelzebub::Terminals;
#else
    #define TerminalBase void
    //  Hue hue hue.
#endif

__extern __bland void InitializeMemory();
__extern __bland void InitializeInterrupts();

__extern __bland TerminalBase * InitializeTerminalProto();
__extern __bland TerminalBase * InitializeTerminalMain();
