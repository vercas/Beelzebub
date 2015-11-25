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

#include <system/registers.hpp>
#include <system/registers_x86.hpp>
#include <system/cpu_instructions.hpp>
#include <system/domain.hpp>
#include <system/msrs.hpp>

#include <synchronization/spinlock.hpp>
#include <execution/thread.hpp>
#include <exceptions.hpp>

#include <synchronization/atomic.hpp>

#define REGFUNC1(regl, regu, type)                                   \
static __forceinline type MCATS2(Get, regu)()                        \
{                                                                    \
    type ret;                                                        \
                                                                     \
    asm volatile ( "mov %%" #regl ", %0\n\t"                         \
                 : "=r"(ret) );                                      \
                                                                     \
    return ret;                                                      \
}                                                                    \
static __forceinline void MCATS2(Set, regu)(const type val)          \
{                                                                    \
    asm volatile ( "mov %0, %%" #regl "\n\t"                         \
                 : : "r"(val) );                                     \
}

#define REGFUNC2(regl, regu, type, type2)                            \
static __forceinline type2 MCATS2(Get, regu)()                       \
{                                                                    \
    type ret;                                                        \
                                                                     \
    asm volatile ( "mov %%" #regl ", %0\n\t"                         \
                 : "=r"(ret) );                                      \
                                                                     \
    return type2(ret);                                               \
}                                                                    \
static __forceinline void MCATS2(Set, regu)(const type val)          \
{                                                                    \
    asm volatile ( "mov %0, %%" #regl "\n\t"                         \
                 : : "r"(val) );                                     \
}                                                                    \
static __forceinline void MCATS2(Set, regu)(const type2 val)         \
{                                                                    \
    type innerVal = val.Value;                                       \
                                                                     \
    asm volatile ( "mov %0, %%" #regl "\n\t"                         \
                 : : "r"(innerVal) );                                \
}

#define REGFUNC3(regl, regu, type)                                   \
static __forceinline type MCATS2(Get, regu)()                        \
{                                                                    \
    type ret;                                                        \
                                                                     \
    asm volatile ( "mov %%" #regl ", %0\n\t"                         \
                 : "=r"(ret) );                                      \
                                                                     \
    return ret;                                                      \
}

namespace Beelzebub { namespace System
{
    /**
     *  The data available to an individual CPU core.
     */
    struct CpuData
    {
        CpuData * SelfPointer;
        size_t Index;

        Domain * DomainDescriptor;
        Tss EmbeddedTss;

        Execution::Thread * ActiveThread;
        ExceptionContext * XContext;

        Exception X;

        uint16_t GdtLength;
        uint16_t TssSegment;
        bool X2ApicMode;
    };

    /**
     *  Represents a processing unit of the system.
     */
    class Cpu
    {
        /*  Constructor(s)  */

    protected:
        Cpu() = default;

    public:
        Cpu(Cpu const &) = delete;
        Cpu & operator =(Cpu const &) = delete;

        /*  Properties  */

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
        static size_t const Count = 1;
#else
        static Synchronization::Atomic<size_t> Count;
#endif

        /*  Segment Registers  */

        REGFUNC3(cs, Cs, seg_t)
        REGFUNC3(ds, Ds, seg_t)
        REGFUNC3(ss, Ss, seg_t)
        REGFUNC3(es, Es, seg_t)
        REGFUNC3(fs, Fs, seg_t)
        REGFUNC3(gs, Gs, seg_t)

        /*  Control Registers  */

        REGFUNC2(cr0, Cr0, creg_t, Beelzebub::System::Cr0)
        REGFUNC1(cr2, Cr2, void *)
        REGFUNC2(cr3, Cr3, creg_t, Beelzebub::System::Cr3)
        REGFUNC1(cr4, Cr4, creg_t)

        /*  Shortcuts  */

        static __forceinline void EnableNxBit()
        {
            const Msr reg = Msr::IA32_EFER;

            asm volatile ( "rdmsr           \n\t"
                           "or $2048, %%eax \n\t" //  That be bit 11.
                           "wrmsr           \n\t"
                         :
                         : "c" (reg)
                         : "eax", "edx" );
        }

        static size_t ComputeIndex();

        /*  CPU-specific data  */

        static __forceinline CpuData * GetData()
        {
            return reinterpret_cast<CpuData *>(CpuInstructions::GsGetPointer(
                offsetof(struct CpuData, SelfPointer)
            ));
        }
    };
}}
