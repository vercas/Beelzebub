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

vaddr_t const volatile Lapic::VirtualAddress = 0xFFFFFFFFFFFFF000;

/*  Addresses  */

paddr_t Lapic::PhysicalAddress = nullpaddr;
//vaddr_t Lapic::VirtualAddress = nullvaddr;

/*  Initialization  */

Handle Lapic::Initialize()
{
    bool const x2ApicSupported = supportsX2APIC();
    Cpu::SetX2ApicMode(x2ApicSupported);

    if (x2ApicSupported)
    {
        auto apicBase = Msrs::GetApicBase();
        
        apicBase.SetX2ApicEnabled(true);

        Msrs::SetApicBase(apicBase);
        //  Now the LAPIC/x2APIC should be usable.
    }

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

        COMPILER_MEMORY_BARRIER();

        return a;
    }
    else
    {
        COMPILER_MEMORY_BARRIER();

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
        COMPILER_MEMORY_BARRIER();

        *((uint32_t *)(((uintptr_t)(uint16_t)reg << 4) + VirtualAddress)) = value;

        COMPILER_MEMORY_BARRIER();
    }
}

/*  Shortcuts  */

void Lapic::SendIpi(LapicIcr icr)
{
    if (Cpu::GetX2ApicMode())
    {
        //  x2APIC mode does not require any checking prior to sending.

        Msr msr = (Msr)((uint32_t)Msr::IA32_X2APIC_BASE
            + (uint32_t)(uint16_t)LapicRegister::InterruptCommandRegisterLow);

        //breakpoint();

        Msrs::Write(msr, icr.Low, icr.High);
    }
    else
    {
        //  The destination field is funky in LAPIC/xAPIC mode. It's the upper
        //  byte...

        icr.SetDestination(icr.GetDestination() << 24);

        //  In xAPIC/LAPIC mode, the delivery status needs to be checked.

        uint32_t volatile low = 0xFFFFFFFFU;

        do
        {
            COMPILER_MEMORY_BARRIER();

            //breakpoint();
            low = *((uint32_t *)(((uintptr_t)(uint16_t)LapicRegister::InterruptCommandRegisterLow  << 4) + VirtualAddress));
        } while (0 != (low & LapicIcr::DeliveryStatusBit));

        COMPILER_MEMORY_BARRIER();

        *((uint32_t *)(((uintptr_t)(uint16_t)LapicRegister::InterruptCommandRegisterHigh << 4) + VirtualAddress)) = icr.High;

        COMPILER_MEMORY_BARRIER();

        *((uint32_t *)(((uintptr_t)(uint16_t)LapicRegister::InterruptCommandRegisterLow  << 4) + VirtualAddress)) = icr.Low ;

        COMPILER_MEMORY_BARRIER();
    }
}
