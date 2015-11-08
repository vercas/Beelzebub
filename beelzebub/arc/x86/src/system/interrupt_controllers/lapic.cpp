#include <system/interrupt_controllers/lapic.hpp>
#include <system/cpu.hpp>
#include <system/msrs.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;

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
    //  Now the LAPIC/x2APIC should be usable.

    auto svr = GetSvr();

    svr.SetSoftwareEnabled(true);
    svr.SetSpuriousVector(0xF0);
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

/*  Shortcuts  */

void Lapic::SendIpi(LapicIcr const icr)
{
    if (Cpu::GetX2ApicMode())
    {
        //  x2APIC mode does not require any checking prior to sending.

        Msr msr = (Msr)((uint32_t)Msr::IA32_X2APIC_BASE
            + (uint32_t)(uint16_t)LapicRegister::InterruptCommandRegisterLow);

        Msrs::Write(msr, icr.Low, icr.High);
    }
    else
    {
        //  In xAPIC/LAPIC mode, the delivery status needs to be checked.

        uint32_t volatile low = 0xFFFFFFFFU;

        do
        {
            msg("<< checking to send IPI>>");

            COMPILER_MEMORY_BARRIER();

            low = *((uint32_t *)(((uintptr_t)(uint16_t)LapicRegister::InterruptCommandRegisterLow  << 4) + VirtualAddress));
        } while (0 != (low & LapicIcr::DeliveryStatusBit));

        msg("<< free to send IPI>>");

        *((uint32_t *)(((uintptr_t)(uint16_t)LapicRegister::InterruptCommandRegisterHigh << 4) + VirtualAddress)) = icr.High;
        msg("<< written HIGH! >>");
        *((uint32_t *)(((uintptr_t)(uint16_t)LapicRegister::InterruptCommandRegisterLow  << 4) + VirtualAddress)) = icr.Low ;
    }
}
