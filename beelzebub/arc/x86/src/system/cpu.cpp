#include <system/cpu.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Synchronization;

/****************
    Cpu class
****************/

/*  Properties  */

#if   !defined(__BEELZEBUB_SETTINGS_NO_SMP)
Atomic<size_t> Cpu::Count {0};
#endif

/*  Operations  */

size_t Cpu::ComputeIndex()
{
    uint32_t a, b, c, d;

    //  Find the standard feature flags.
    CpuId::Execute(0x00000001U, a, b, c, d);

    return (b & 0xFF000000U) >> 24;
}
