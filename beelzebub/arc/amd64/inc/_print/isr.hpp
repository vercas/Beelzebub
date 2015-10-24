#pragma once

#include <system/isr.hpp>
#include <terminals/base.hpp>

namespace Beelzebub { namespace System
{
    __cold __bland __noinline Beelzebub::Terminals::TerminalWriteResult PrintToTerminal(Beelzebub::Terminals::TerminalBase * const term, IsrState const * const val);
    __cold __bland __noinline Beelzebub::Terminals::TerminalWriteResult PrintToDebugTerminal(IsrState const * const val);
}}
