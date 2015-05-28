#include <system/cpu.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

/****************
    Cpu class
****************/

/*  Operations  */

size_t Cpu::GetUnpreciseIndex()
{
    uint32_t a, b, c, d;

    //  Find the standard feature flags.
    CpuId::Execute(0x00000001U, a, b, c, d);

    return (b & 0xFF000000U) >> 24;
}
