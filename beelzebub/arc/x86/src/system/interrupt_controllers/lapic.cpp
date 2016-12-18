/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#include <system/interrupt_controllers/lapic.hpp>

#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE)
#include <system/cpu.hpp>
#endif

#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE) || defined(__BEELZEBUB_SETTINGS_APIC_MODE_X2APIC)
#include <system/msrs.hpp>
#endif

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;

static bool supportsX2APIC()
{
    //  A precomputed CPUID structure is not the right tool for the job.
    //  Only one bit is of interest, thus only that bit is obtained.

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

/*  Ender  */

void Lapic::IrqEnder(INTERRUPT_ENDER_ARGS)
{
    (void)handler;
    (void)vector;

    EndOfInterrupt();
}

/*  Initialization  */

Handle Lapic::Initialize()
{
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE) || defined(__BEELZEBUB_SETTINGS_APIC_MODE_X2APIC)
    bool const x2ApicSupported = supportsX2APIC();
#endif

#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE)
    Cpu::GetData()->X2ApicMode = x2ApicSupported;
    //  This is just a CPU-specific boolean value. Doesn't actually enable
    //  x2APIC mode, or disable it for that matter.

    if (x2ApicSupported)
    {
        auto apicBase = Msrs::GetApicBase();
        //  ApicBase -> IA32_APIC_BASE which is NOT the base physical address!

        apicBase.SetX2ApicEnabled(true);
        //  Just enables x2APIC mode. Nothing else!

        Msrs::SetApicBase(apicBase);
        //  Now the LAPIC/x2APIC should be usable.
    }
#elif defined(__BEELZEBUB_SETTINGS_APIC_MODE_X2APIC)
    ASSERT(x2ApicSupported);

    auto apicBase = Msrs::GetApicBase();

    apicBase.SetX2ApicEnabled(true);

    Msrs::SetApicBase(apicBase);
#endif

    auto svr = GetSvr();

    svr.SetSoftwareEnabled(true);
    svr.SetSpuriousVector(0xF0);
    SetSvr(svr);

    Cpu::GetData()->LapicId = GetId();

    return HandleResult::Okay;
}

/*  Registers  */

uint32_t Lapic::ReadRegister(LapicRegister const reg)
{
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE)
    if (Cpu::GetData()->X2ApicMode)
    {
#endif
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE) || defined(__BEELZEBUB_SETTINGS_APIC_MODE_X2APIC)
        uint32_t a, d;
        Msr const msr = (Msr)((uint32_t)Msr::IA32_X2APIC_BASE + (uint32_t)(uint16_t)reg);

        Msrs::Read(msr, a, d);

        COMPILER_MEMORY_BARRIER();

        return a;
#endif
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE)
    }
#endif
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE) || defined(__BEELZEBUB_SETTINGS_APIC_MODE_LEGACY)
    COMPILER_MEMORY_BARRIER();

    return *((uint32_t *)(((uintptr_t)(uint16_t)reg << 4) + VirtualAddress));
#endif
}

void Lapic::WriteRegister(LapicRegister const reg, uint32_t const value)
{
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE)
    if (Cpu::GetData()->X2ApicMode)
    {
#endif
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE) || defined(__BEELZEBUB_SETTINGS_APIC_MODE_X2APIC)
        uint32_t d = 0;
        Msr msr = (Msr)((uint32_t)Msr::IA32_X2APIC_BASE + (uint32_t)(uint16_t)reg);

        return Msrs::Write(msr, value, d);
#endif
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE)
    }
#endif
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE) || defined(__BEELZEBUB_SETTINGS_APIC_MODE_LEGACY)
    COMPILER_MEMORY_BARRIER();

    *((uint32_t *)(((uintptr_t)(uint16_t)reg << 4) + VirtualAddress)) = value;

    COMPILER_MEMORY_BARRIER();
#endif
}

/*  Shortcuts  */

void Lapic::SendIpi(LapicIcr icr)
{
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE)
    if (Cpu::GetData()->X2ApicMode)
    {
#endif
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE) || defined(__BEELZEBUB_SETTINGS_APIC_MODE_X2APIC)
        //  x2APIC mode does not require any checking prior to sending.

        Msr msr = (Msr)((uint32_t)Msr::IA32_X2APIC_BASE
            + (uint32_t)(uint16_t)LapicRegister::InterruptCommandRegisterLow);

        //breakpoint();

        return Msrs::Write(msr, icr.Low, icr.High);
#endif
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE)
    }
#endif
#if defined(__BEELZEBUB_SETTINGS_APIC_MODE_FLEXIBLE) || defined(__BEELZEBUB_SETTINGS_APIC_MODE_LEGACY)

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

    *((uint32_t *)(((uintptr_t)(uint16_t)LapicRegister::InterruptCommandRegisterLow  << 4) + VirtualAddress)) = icr.Low;

    COMPILER_MEMORY_BARRIER();
#endif
}
