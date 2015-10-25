#pragma once

#include <terminals/base.hpp>

#define PRINT_FUNCS(type) \
__cold __bland __noinline Beelzebub::Terminals::TerminalWriteResult PrintToTerminal(Beelzebub::Terminals::TerminalBase * const term, type val); \
__cold __bland __noinline Beelzebub::Terminals::TerminalWriteResult PrintToDebugTerminal(type val);

#define PRINT_FUNCS_CONST(type) \
__cold __bland __noinline Beelzebub::Terminals::TerminalWriteResult PrintToTerminal(Beelzebub::Terminals::TerminalBase * const term, type const val); \
__cold __bland __noinline Beelzebub::Terminals::TerminalWriteResult PrintToDebugTerminal(type const val);
