#pragma once

#include <system/registers_x86.hpp>
#include <terminals/base.hpp>

namespace Beelzebub { namespace System
{
    __cold __bland __noinline Terminals::TerminalWriteResult PrintToTerminal(Terminals::TerminalBase * const term, Ia32Efer const val);
}}
