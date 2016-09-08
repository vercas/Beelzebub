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

#pragma once

#include <system/lapic_registers.hpp>
#include <beel/handles.h>

#define LAPICREGFUNC1(name, prettyName, type)                             \
static __forceinline type MCATS2(Get, prettyName)()               \
{                                                                         \
    return type(ReadRegister(LapicRegister::name));                       \
}                                                                         \
static __forceinline void MCATS2(Set, prettyName)(const type val) \
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

        static __cold Handle Initialize();

        /*  Registers  */

        static __hot uint32_t ReadRegister(LapicRegister const reg);
        static __hot void WriteRegister(LapicRegister const reg, uint32_t const value);

        /*  Shortcuts  */

        static __forceinline uint32_t GetId()
        {
            return ReadRegister(LapicRegister::LapicId);
        }

        static __hot __forceinline void EndOfInterrupt()
        {
            WriteRegister(LapicRegister::EndOfInterrupt, 0);
        }

        static void SendIpi(LapicIcr icr);

        LAPICREGFUNC1(SpuriousInterruptVector, Svr, LapicSvr)
    };
}}}
