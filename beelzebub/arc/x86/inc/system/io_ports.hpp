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

#include <metaprogramming.h>

namespace Beelzebub { namespace System
{
    /**
     *  Represents the interrupt state of the system
     */
    class Io
    {
        /*  Constructor(s)  */

    protected:
        Io() = default;

    public:
        Io(Io const &) = delete;
        Io & operator =(Io const &) = delete;

        /*  Port I/O  */

        static __forceinline void Out8(uint16_t const port
                                             , uint8_t const value)
        {
            asm volatile ( "outb %1, %0 \n\t"
                         :
                         : "dN" (port), "a" (value) );
        }

        static __forceinline void Out8n(uint16_t const port
                                              , uint8_t const * src
                                              , size_t const count)
        {
            asm volatile ( "rep outsb \n\t"
                         : "+S" (src)
                         : "d" (port) );
        }

        static __forceinline void Out16(uint16_t const port
                                              , uint16_t const value)
        {
            asm volatile ( "outw %1, %0 \n\t"
                         :
                         : "dN" (port), "a" (value) );
        }

        static __forceinline void Out32(uint16_t const port
                                              , uint32_t const value)
        {
            asm volatile ( "outl %1, %0 \n\t"
                         :
                         : "dN" (port), "a" (value) );
        }


        static __forceinline void In8(uint16_t const port, uint8_t & value)
        {
            asm volatile ( "inb %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );
        }

        static __forceinline void In16(uint16_t const port, uint16_t & value)
        {
            asm volatile ( "inw %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );
        }

        static __forceinline void In32(uint16_t const port, uint32_t & value)
        {
            asm volatile ( "inl %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );
        }


        static __forceinline uint8_t In8(uint16_t const port)
        {
            uint8_t value;

            asm volatile ( "inb %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );

            return value;
        }

        static __forceinline uint16_t In16(uint16_t const port)
        {
            uint16_t value;

            asm volatile ( "inw %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );

            return value;
        }

        static __forceinline uint32_t In32(uint16_t const port)
        {
            uint32_t value;

            asm volatile ( "inl %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );

            return value;
        }
    };
}}
