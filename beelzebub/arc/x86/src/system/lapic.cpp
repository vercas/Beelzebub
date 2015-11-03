#include <system/lapic.hpp>
#include <system/cpu.hpp>
#include <system/msrs.hpp>
#include <entry.h>

using namespace Beelzebub;
using namespace Beelzebub::System;

static bool supportsX2APIC()
{
    //  No, we ain't computing the whole CPUID shenanigans for the sake of one
    //  flag. We get what we want.

    uint32_t cpuidLeaf = 0x00000001U, ecx, dummy;

    asm volatile ( "cpuid"
                 : "+a" (cpuidLeaf), "=b" (dummy), "=c" (ecx), "=d" (dummy));
    //  Yes, this will trash `cupidLeaf`.

    return 0 != ((1 << 21) & ecx);
}

/******************
    Lapic class
******************/

/*  Addresses  */

paddr_t Lapic::PhysicalAddress = nullpaddr;
//vaddr_t Lapic::VirtualAddress = nullvaddr;

/*  Initialization  */

Handle Lapic::Initialize()
{
    auto apicBase = Msrs::GetApicBase();
    bool const x2ApicSupported = supportsX2APIC();

    apicBase.SetGlobalLapicEnabled(true);   //  Just makin' sure.
    apicBase.SetX2ApicEnabled(x2ApicSupported);

    Cpu::SetX2ApicMode(x2ApicSupported);
    Msrs::SetApicBase(apicBase);
    //  Now the LAPIC should be usable.

    auto svr = GetSvr();

    svr.SetSoftwareEnabled(true);
    svr.SetSpuriousVector(32);
    SetSvr(svr);

    return HandleResult::Okay;
}

/*  Registers  */

uint32_t Lapic::ReadRegister(LapicRegister const reg)
{
    if (Cpu::GetX2ApicMode())
    {
        uint32_t a, d;
        Msr msr = (Msr)((uint32_t)Msr::IA32_X2APIC_BASE + (uint32_t)(uint16_t)reg);

        Msrs::Read(msr, a, d);

        return a;
    }
    else
    {
        return *((uint32_t *)(((uintptr_t)(uint16_t)reg << 4) + VirtualAddress));
    }
}

void Lapic::WriteRegister(LapicRegister const reg, uint32_t const value)
{
    if (Cpu::GetX2ApicMode())
    {
        uint32_t d = 0;
        Msr msr = (Msr)((uint32_t)Msr::IA32_X2APIC_BASE + (uint32_t)(uint16_t)reg);

        Msrs::Write(msr, value, d);
    }
    else
    {
        *((uint32_t *)(((uintptr_t)(uint16_t)reg << 4) + VirtualAddress)) = value;
    }
}
