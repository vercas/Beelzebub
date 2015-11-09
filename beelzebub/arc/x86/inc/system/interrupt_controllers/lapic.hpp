#pragma once

#include <system/lapic_registers.hpp>
#include <handles.h>

#define LAPICREGFUNC1(name, prettyName, type)                             \
static __bland __forceinline type MCATS2(Get, prettyName)()               \
{                                                                         \
    return type(ReadRegister(LapicRegister::name));                       \
}                                                                         \
static __bland __forceinline void MCATS2(Set, prettyName)(const type val) \
{                                                                         \
    WriteRegister(LapicRegister::name, val.Value);                        \
}

namespace Beelzebub { namespace System { namespace InterruptControllers
{
    /**
     *  <summary>Contains methods for interacting with the local APIC.</summary>
     */
    class Lapic
    {
    public:
        /*  Addresses  */

        static paddr_t PhysicalAddress;
        static vaddr_t const volatile VirtualAddress;
        //  Very last page - why not?

        /*  Constructor(s)  */

    protected:
        Lapic() = default;

    public:
        Lapic(Lapic const &) = delete;
        Lapic & operator =(Lapic const &) = delete;

        /*  Initialization  */

        static __cold __bland Handle Initialize();

        /*  Registers  */

        static __hot __bland uint32_t ReadRegister(LapicRegister const reg);
        static __hot __bland void WriteRegister(LapicRegister const reg, uint32_t const value);

        /*  Shortcuts  */

        static __bland __forceinline uint32_t GetId()
        {
            return ReadRegister(LapicRegister::LapicId);
        }

        static __hot __bland __forceinline void EndOfInterrupt()
        {
            WriteRegister(LapicRegister::EndOfInterrupt, 0);
        }

        static __bland void SendIpi(LapicIcr icr);

        LAPICREGFUNC1(SpuriousInterruptVector, Svr, LapicSvr)
    };
}}}
