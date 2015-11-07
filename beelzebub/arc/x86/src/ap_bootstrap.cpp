#include <ap_bootstrap.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

GdtRegister KernelGdtPointer {0, nullptr};
uintptr_t ApStackTopPointer = nullpaddr;
uint32_t ApInitializationLock = 0;
