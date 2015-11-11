#include <ap_bootstrap.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

GdtRegister KernelGdtPointer {0, nullptr};
uintptr_t ApStackTopPointer = nullpaddr;

uint32_t volatile ApInitializationLock1 = 0;
uint32_t volatile ApInitializationLock2 = 0;
uint32_t volatile ApInitializationLock3 = 0;
