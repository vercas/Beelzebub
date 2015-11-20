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

#include <system/isr.hpp>
#include <system/idt.hpp>

namespace Beelzebub { namespace System
{
    /**
     *  Represents the interrupt state of the system
     */
    class Interrupts
    {
    public:
        /*  Statics  */

        static size_t const Count = 256;
        static size_t const StubSize = 16;

        static Idt Table;
        static IdtRegister Register;

        /*  Constructor(s)  */

    protected:
        Interrupts() = default;

    public:
        Interrupts(Interrupts const &) = delete;
        Interrupts & operator =(Interrupts const &) = delete;

        /*  Interrupts  */

        static __bland __forceinline bool AreEnabled()
        {
            size_t flags;

            asm volatile ( "pushf        \n\t"
                           "pop %[flags] \n\t"
                           : [flags]"=r"(flags) );
            //  Push and pop don't change any flags. Yay!

            return (flags & (size_t)(1 << 9)) != 0;
        }

        static __bland __forceinline void Enable()
        {
            asm volatile ( "sti \n\t" : : : "memory" );
            //  This is a memory barrier to prevent the compiler from moving things around it.
        }

        static __bland __forceinline void Disable()
        {
            asm volatile ( "cli \n\t" : : : "memory" );
            //  This is a memory barrier to prevent the compiler from moving things around it.
        }

        /**
         *  <summary>
         *  Disables interrupts and returns a cookie which allows restoring the
         *  interrupt state as it was before executing this function.
         *  </summary>
         *  <return>
         *  A cookie which allows restoring the interrupt state as it was before
         *  executing this function.
         *  </return>
         */
        static __bland __forceinline int_cookie_t PushDisable()
        {
            int_cookie_t cookie;

            asm volatile ( "pushf      \n\t"
                           "cli        \n\t" // Yes, do it as soon as possible, to avoid interruption.
                           "pop %[dst] \n\t"
                         : [dst]"=r"(cookie)
                         :
                         : "memory");
            
            /*  A bit of wisdom from froggey: ``On second thought, the constraint for flags ["cookie"] in
                interrupt_disable [PushDisableInterrupts] should be "=r", not "=rm". If the compiler decided
                to store [the] flags on the stack and generated an (E|R)SP-relative address, the address would
                end up being off by 4/8 [when passed onto the pop instruction because the stack pointer changed].'' */

            return cookie;
        }

        /**
         *  <summary>
         *  Enables interrupts and returns a cookie which allows restoring the
         *  interrupt state as it was before executing this function.
         *  </summary>
         *  <return>
         *  A cookie which allows restoring the interrupt state as it was before
         *  executing this function.
         *  </return>
         */
        static __bland __forceinline int_cookie_t PushEnable()
        {
            int_cookie_t cookie;

            asm volatile ( "pushf      \n\t"
                           "sti        \n\t"
                           "pop %[dst] \n\t"
                         : [dst]"=r"(cookie)
                         :
                         : "memory");
            
            return cookie;
        }

        /**
         *  <summary>
         *  Restores interrupt state based on the given cookie.
         *  </summary>
         *  <return>True if interrupts are now enabled, otherwise false.</return>
         */
        static __bland __forceinline bool RestoreState(const int_cookie_t cookie)
        {
            asm volatile ( "push %[src] \n\t"   //  PUT THE COOKIE DOWN!
                           "popf        \n\t"
                         :
                         : [src]"rm"(cookie)
                         : "memory", "cc" );

            //  Here the cookie can safely be retrieved from the stack because
            //  RSP will change after push, not before.

            return (cookie & (int_cookie_t)(1 << 9)) == 0;
        }
    };

    #define INTERRUPT_ENDER_ARGS                                \
          Beelzebub::System::IsrState * const state             \
        , void const * const handler                            \
        , uint8_t const vector

    typedef void (*InterruptEnderFunction)(INTERRUPT_ENDER_ARGS);

    #define INTERRUPT_HANDLER_ARGS                              \
          Beelzebub::System::IsrState * const state             \
        , Beelzebub::System::InterruptEnderFunction const ender \
        , void const * const handler                            \
        , uint8_t const vector

    typedef void (*InterruptHandlerFunction)(INTERRUPT_HANDLER_ARGS);

    #define END_OF_INTERRUPT()                 \
        do                                     \
        {                                      \
            if (ender != nullptr)              \
                ender(state, handler, vector); \
        } while (false)

    /**
     *  The actual IDT.
     */
    __extern uint64_t IsrGates[Interrupts::Count];

    /**
     *  Array of higher-level interrupt handlers.
     */
    __extern InterruptHandlerFunction InterruptHandlers[Interrupts::Count];

    /**
     *  Array of higher-level interrupt handlers.
     */
    __extern InterruptEnderFunction InterruptEnders[Interrupts::Count];
}}
