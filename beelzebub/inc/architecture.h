#pragma once

#include <metaprogramming.h>

#ifdef __cplusplus
    #include <terminals/base.hpp>

    using namespace Beelzebub::Terminals;
#else
    #define TerminalBase void
    //  Hue hue hue.
#endif

__extern __noinline __bland void InitializeMemory();
__extern __noinline __bland void InitializeInterrupts();

__extern __noinline __bland TerminalBase * InitializeTerminalProto();
__extern __noinline __bland TerminalBase * InitializeTerminalMain();

#ifdef __BEELZEBUB__TEST_MT
__extern __noinline __bland void StartMultitaskingTest();
#endif
