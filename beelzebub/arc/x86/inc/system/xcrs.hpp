/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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

#include <system/registers_x86.hpp>

#define XCRFUNC1(number, type)                                    \
static __forceinline type MCATS2(Get, number)()                   \
{                                                                 \
    uint64_t temp = 0;                                            \
    Read(number, temp);                                           \
    return type(temp);                                            \
}                                                                 \
static __forceinline void MCATS2(Set, number)(type const val)     \
{                                                                 \
    Write(number, val.Value);                                     \
}

#define XCRFUNC2(number, type)                                    \
static __forceinline type MCATS2(Get, number)()                   \
{                                                                 \
    uint64_t temp = 0;                                            \
    Read(number, temp);                                           \
    return reinterpret_cast<type>(temp);                          \
}                                                                 \
static __forceinline void MCATS2(Set, number)(type const val)     \
{                                                                 \
    Write(number, reinterpret_cast<uint64_t>(val));               \
}

namespace Beelzebub { namespace System
{
    /**
     *  Contains functions for interacting with extended control registers.
     */
    class Xcrs
    {
        /*  Constructor(s)  */

    protected:
        Xcrs() = default;

    public:
        Xcrs(Xcrs const &) = delete;
        Xcrs & operator =(Xcrs const &) = delete;

        /*  MSRs  */

        static __forceinline uint64_t Read64(const uint32_t reg)
        {
            uint32_t a, d;

            asm volatile ( "xgetbv \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            return (uint64_t)a | ((uint64_t)d << 32);
        }

        static __forceinline void Read(const uint32_t reg, uint32_t & a, uint32_t & d)
        {
            asm volatile ( "xgetbv \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );
        }

        static __forceinline void Read(const uint32_t reg, uint64_t & val)
        {
            uint32_t a, d;

            asm volatile ( "xgetbv \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            val = (uint64_t)a | ((uint64_t)d << 32);
        }

        static __forceinline void Write(const uint32_t reg, const uint32_t a, const uint32_t d)
        {
            asm volatile ( "xsetbv \n\t"
                         : 
                         : "c" (reg), "a" (a), "d" (d) );
        }

        static __forceinline void Write(const uint32_t reg, const uint64_t val)
        {
            register uint32_t a asm("eax") = (uint32_t)val;
            register uint32_t d asm("edx") = (uint32_t)(val >> 32);

            asm volatile ( "xsetbv \n\t"
                         : 
                         : "c" (reg), "a" (a), "d" (d) );
        }

        // XCRFUNC1(0, 0, Xcr0)
    };
}}
