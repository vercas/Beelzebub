#pragma once

#include <metaprogramming.h>

namespace Beelzebub { namespace System
{
    /**
     *  Represents the interrupt state of the system
     */
    class Interrupts
    {
    public:

        /*  Interrupts  */

        //static_assert(4 == offsetof(struct CpuData, InterruptDisableCount));
        //  Protecting against accidental changes of the CpuData struct without updating the appropriate functions.

        static __bland __forceinline bool AreEnabled()
        {
            size_t flags;

            asm volatile ( "pushf\n\t"
                           "pop %0\n\t"
                           : "=r"(flags) );
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
         *  Pushes the interrupt disabling counter and disables interrupts
         *
         *  @return Value of interrupt counter prior to calling the method.
         *-/
        static __bland __forceinline uint32_t PushDisableInterrupts()
        {
#if   defined(__BEELZEBUB__ARCH_AMD64)
            register size_t ret asm("rax") = 1;
#else
            register size_t ret asm("eax") = 1;
#endif

            asm volatile ( "cli \n\t"
                           "xaddl %%eax, %%gs:($4) \n\t"
                         : "+a"(ret)
                         :
                         : "memory" );
            //  This is a memory barrier to prevent the compiler from moving things around it.

            return (uint32_t)ret;
        }//*/

        /**
         *  Disables interrupts and returns a cookie which allows restoring the interrupt state
         *  as it was before executing this function.
         *
         *  @return A cookie which allows restoring the interrupt state as it was before executing this function.
         */
        static __bland __forceinline int_cookie_t PushDisable()
        {
            int_cookie_t cookie;

            asm volatile ( "pushf  \n\t"
                           "cli    \n\t" // Yes, do it as soon as possible, to avoid interruption.
                           "pop %0 \n\t"
                         : "=r"(cookie)
                         :
                         : "memory");
            
            /*  A bit of wisdom from froggey: ``On second thought, the constraint for flags ["cookie"] in
                interrupt_disable [PushDisableInterrupts] should be "=r", not "=rm". If the compiler decided
                to store [the] flags on the stack and generated an (E|R)SP-relative address, the address would
                end up being off by 4/8 [when passed onto the pop instruction because the stack pointer changed].'' */

            return cookie;
        }

        /**
         *  Restores interrupt state based on the given cookie.
         *
         *  @return True if interrupts are now enabled, otherwise false.
         */
        static __bland __forceinline bool RestoreState(const int_cookie_t cookie)
        {
            asm volatile ( "push %0 \n\t"   //  PUT THE COOKIE DOWN!
                           "popf    \n\t"
                         :
                         : "rm"(cookie)
                         : "memory", "cc" );

            //  Here the cookie can safely be retrieved from the stack because RSP will change after
            //  push, not before.

            return (cookie & (int_cookie_t)(1 << 9)) == 0;
        }
    };
}}
