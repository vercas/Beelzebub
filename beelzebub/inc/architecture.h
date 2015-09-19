#pragma once

#include <metaprogramming.h>
#include <handles.h>

#ifdef __cplusplus
    #include <terminals/base.hpp>

    using namespace Beelzebub;
    using namespace Beelzebub::Terminals;
#else
    #define TerminalBase void
    //  Hue hue hue.
#endif

__extern __noinline __bland Handle InitializeMemory();
__extern __noinline __bland Handle InitializeModules();
__extern __noinline __bland Handle InitializeInterrupts();

__extern __noinline __bland TerminalBase * InitializeTerminalProto();
__extern __noinline __bland TerminalBase * InitializeTerminalMain();

#ifdef __BEELZEBUB__TEST_MT
__extern __noinline __bland void StartMultitaskingTest();
#endif
