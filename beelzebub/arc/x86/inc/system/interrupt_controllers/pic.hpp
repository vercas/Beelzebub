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

namespace Beelzebub { namespace System { namespace InterruptControllers
{
    /**
     *  <summary>Contains methods for interacting with the PIC.</summary>
     */
    class Pic
    {
    public:
        /*  Statics  */

        static uint16_t const MasterCommandPort = 0x20;
        static uint16_t const MasterDataPort = 0x21;
        static uint16_t const SlaveCommandPort = 0xA0;
        static uint16_t const SlaveDataPort = 0xA1;

        static uint8_t VectorOffset;
        static bool Active;

        /*  Ender  */

        static __hot void IrqEnder(INTERRUPT_ENDER_ARGS);

        /*  Constructor(s)  */

    protected:
        Pic() = default;

    public:
        Pic(Pic const &) = delete;
        Pic & operator =(Pic const &) = delete;

        /*  (De)initialization  */

        static __cold void Initialize(uint8_t const vecOff);
        static __cold void Disable();

        /*  Subscription  */

        static bool Subscribe(uint8_t const irq, InterruptHandlerPartialFunction const handler, bool const unmask = false);
        static bool Subscribe(uint8_t const irq, InterruptHandlerFullFunction const handler, bool const unmask = false);
        
        static bool Unsubscribe(uint8_t const irq, bool const mask = false);
        static bool IsSubscribed(uint8_t const irq);

        /*  Masking  */

        static bool SetMasked(uint8_t const irq, bool const masked);
        static bool GetMasked(uint8_t const irq);
    };
}}}
