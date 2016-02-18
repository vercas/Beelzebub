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

#include <system/interrupts.hpp>
#include <utils/bitfields.hpp>

namespace Beelzebub { namespace System { namespace InterruptControllers
{
    /**
     *  <summary>
     *  Represents the contents of the ID register of the I/O APIC.
     *  </summary>
     */
    struct IoapicIdRegister
    {
        /*  Bit structure:
         *       0 -  23 : Reserved (must be 0)
         *      24 -  27 : I/O APIC ID
         *      28 -  31 : Reserved (must be 0)
         */

        /*  Properties  */

        BITFIELD_DEFAULT_4WEx(24,  4, uint8_t, Id, 32)

        /*  Constructor  */

        /**
         *  <summary>
         *  Creates a new I/O APIC ID register structure from the given
         *  raw value.
         *  </summary>
         */
        inline explicit constexpr IoapicIdRegister(uint32_t const val)
            : Value(val)
        {
            
        }

        /*  Field(s)  */

    //private:

        uint32_t Value;
    };

    /**
     *  <summary>Contains methods for interacting with the I/O APIC.</summary>
     */
    enum class IoapicRegisters : uint8_t
    {
        Id                      = 0x00,
        Version                 = 0x01,
        ArbitrationId           = 0x02,

        RedirectionTableStart   = 0x10,
    };
    
    /**
     *  <summary>Contains methods for interacting with the I/O APIC.</summary>
     */
    class Ioapic
    {
    public:
        /*  Statics  */

        static size_t const Limit = 16;
        static size_t Count;
        static Ioapic All[Limit];

        /*  Ender  */

        static __hot void IrqEnder(INTERRUPT_ENDER_ARGS);

        /*  Constructor(s)  */

    protected:
        inline constexpr Ioapic()
            : Id()
            , RegisterSelector()
            , RegisterWindow()
            , GlobalIrqBase()
            , VectorOffset()
        {

        }

    public:
        inline constexpr Ioapic(uint8_t const id
                                      , uintptr_t const addr
                                      , uint32_t const globalBase
                                      , uint8_t const vecOff)
            : Id(id)
            , RegisterSelector(addr + 0x00)
            , RegisterWindow(addr + 0x10)
            , GlobalIrqBase(globalBase)
            , VectorOffset(vecOff)
        {

        }

        Ioapic(Ioapic const &) = delete;
        Ioapic & operator =(Ioapic const &) = delete;

        /*  (De)initialization  */

        __cold void Initialize();

        /*  Fields  */

        uint8_t   const Id;
        uintptr_t const RegisterSelector;
        uintptr_t const RegisterWindow;
        uint32_t  const GlobalIrqBase;
        uint8_t   VectorOffset;
    };
}}}
