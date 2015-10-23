#pragma once

#include <metaprogramming.h>

namespace Beelzebub { namespace System
{
    /**
     *  Represents the interrupt state of the system
     */
    class Io
    {
    public:
        /*  Port I/O  */

        static __bland __forceinline void Out8(uint16_t const port
                                             , uint8_t const value)
        {
            asm volatile ( "outb %1, %0 \n\t"
                         :
                         : "dN" (port), "a" (value) );
        }

        static __bland __forceinline void Out16(uint16_t const port
                                              , uint16_t const value)
        {
            asm volatile ( "outw %1, %0 \n\t"
                         :
                         : "dN" (port), "a" (value) );
        }

        static __bland __forceinline void Out32(uint16_t const port
                                              , uint32_t const value)
        {
            asm volatile ( "outl %1, %0 \n\t"
                         :
                         : "dN" (port), "a" (value) );
        }


        static __bland __forceinline void In8(uint16_t const port, uint8_t & value)
        {
            asm volatile ( "inb %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );
        }

        static __bland __forceinline void In16(uint16_t const port, uint16_t & value)
        {
            asm volatile ( "inw %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );
        }

        static __bland __forceinline void In32(uint16_t const port, uint32_t & value)
        {
            asm volatile ( "inl %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );
        }


        static __bland __forceinline uint8_t In8(uint16_t const port)
        {
            uint8_t value;

            asm volatile ( "inb %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );

            return value;
        }

        static __bland __forceinline uint16_t In16(uint16_t const port)
        {
            uint16_t value;

            asm volatile ( "inw %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );

            return value;
        }

        static __bland __forceinline uint32_t In32(uint16_t const port)
        {
            uint32_t value;

            asm volatile ( "inl %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );

            return value;
        }
    };
}}
