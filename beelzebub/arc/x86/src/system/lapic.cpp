#include <system/lapic.hpp>
#include <system/msrs.hpp>
#include <entry.h>

using namespace Beelzebub;
using namespace Beelzebub::System;

static bool x2ApicEnabled = false;

/******************
    Lapic class
******************/

/*  Initialization  */

void Lapic::Initialize()
{
    x2ApicEnabled = 0 != (JG_INFO_ROOT_EX->flags & JG_INFO_FLAG_X2APIC);
    //  I am not going to be a phallus like some CPU manufacturers
    //  who sell chips with broken x2APICs.
}

/*  Registers  */

uint32_t Lapic::ReadRegister(LapicRegister const reg)
{
    if (x2ApicEnabled)
    {
        uint32_t a, d;
        Msr msr = (Msr)((uint32_t)Msr::IA32_X2APIC_BASE + (uint32_t)(uint16_t)reg);

        Msrs::Read(msr, a, d);

        return a;
    }
    else
    {
        return *((uint32_t *)(((uintptr_t)(uint16_t)reg << 4) + JG_INFO_ROOT_EX->lapic_paddr));
    }
}

void Lapic::WriteRegister(LapicRegister const reg, uint32_t const value)
{
    if (x2ApicEnabled)
    {
        uint32_t d = 0;
        Msr msr = (Msr)((uint32_t)Msr::IA32_X2APIC_BASE + (uint32_t)(uint16_t)reg);

        Msrs::Write(msr, value, d);
    }
    else
    {
        *((uint32_t *)(((uintptr_t)(uint16_t)reg << 4) + JG_INFO_ROOT_EX->lapic_paddr)) = value;
    }
}
