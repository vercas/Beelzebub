#pragma once

#include <system/gdt.hpp>

__extern Beelzebub::System::GdtRegister KernelGdtPointer;
__extern uintptr_t ApStackTopPointer;
__extern uint32_t ApInitializationLock;
__extern int ApBreakpointCookie;

__extern long ApBootstrapBegin;	//	Only address of this symbol is used.
__extern long ApBootstrapEnd;	//	Only address of this symbol is used.

__extern paddr_t BootstrapPml4Address;
