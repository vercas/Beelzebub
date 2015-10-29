#include <system/lapic.hpp>
#include <system/cpu.hpp>
#include <system/msrs.hpp>
#include <entry.h>

using namespace Beelzebub;
using namespace Beelzebub::System;

/******************
    Lapic class
******************/

/*  Initialization  */

void Lapic::Initialize()
{
    //  Uh...?
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
        return *((uint32_t *)(((uintptr_t)(uint16_t)reg << 4) + Cpu::GetLapicAddress()));
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
        *((uint32_t *)(((uintptr_t)(uint16_t)reg << 4) + Cpu::GetLapicAddress())) = value;
    }
}
