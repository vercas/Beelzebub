#pragma once

#include <system/gdt.hpp>

__extern Beelzebub::System::GdtRegister KernelGdtPointer;
__extern uintptr_t ApStackTopPointer;
__extern int ApBreakpointCookie;

__extern uint32_t volatile ApInitializationLock1;
__extern uint32_t volatile ApInitializationLock2;
__extern uint32_t volatile ApInitializationLock3;

__extern long ApBootstrapBegin;	//	Only address of this symbol is used.
__extern long ApBootstrapEnd;	//	Only address of this symbol is used.

__extern paddr_t BootstrapPml4Address;
