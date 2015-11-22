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

#include <synchronization/atomic.hpp>

#define REGFUNC1(regl, regu, type)                                   \
static __forceinline type MCATS2(Get, regu)()                \
{                                                                    \
    type ret;                                                        \
                                                                     \
    asm volatile ( "mov %%" #regl ", %0\n\t"                         \
                 : "=r"(ret) );                                      \
                                                                     \
    return ret;                                                      \
}                                                                    \
static __forceinline void MCATS2(Set, regu)(const type val)  \
{                                                                    \
    asm volatile ( "mov %0, %%" #regl "\n\t"                         \
                 : : "r"(val) );                                     \
}

#define REGFUNC2(regl, regu, type, type2)                            \
static __forceinline type2 MCATS2(Get, regu)()               \
{                                                                    \
    type ret;                                                        \
                                                                     \
    asm volatile ( "mov %%" #regl ", %0\n\t"                         \
                 : "=r"(ret) );                                      \
                                                                     \
    return type2(ret);                                               \
}                                                                    \
static __forceinline void MCATS2(Set, regu)(const type val)  \
{                                                                    \
    asm volatile ( "mov %0, %%" #regl "\n\t"                         \
                 : : "r"(val) );                                     \
}                                                                    \
static __forceinline void MCATS2(Set, regu)(const type2 val) \
{                                                                    \
    type innerVal = val.Value;                                       \
                                                                     \
    asm volatile ( "mov %0, %%" #regl "\n\t"                         \
                 : : "r"(innerVal) );                                \
}

#define REGFUNC3(regl, regu, type)                                   \
static __forceinline type MCATS2(Get, regu)()                \
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
        size_t Index;
        Domain * DomainDescriptor;

        Execution::Thread * ActiveThread;
        Synchronization::spinlock_t HeapSpinlock;
        Synchronization::Spinlock<> * HeapSpinlockPointer;

        uint16_t GdtLength;
        bool X2ApicMode;
    } __packed __aligned_nat;

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

        static const size_t CpuDataSize = sizeof(CpuData);

        static __forceinline size_t GetIndex()
        {
            return CpuInstructions::GsGetSize(offsetof(struct CpuData, Index));
        }
        static __forceinline size_t SetIndex(const size_t val)
        {
            return (size_t)CpuInstructions::GsSetSize(offsetof(struct CpuData, Index), val);
        }

        static __forceinline Domain * GetDomain()
        {
            return (Domain *)CpuInstructions::GsGetPointer(offsetof(struct CpuData, DomainDescriptor));
        }
        static __forceinline Domain * SetDomain(Domain * const val)
        {
            return (Domain *)CpuInstructions::GsSetPointer(offsetof(struct CpuData, DomainDescriptor), (uintptr_t)val);
        }

        static __forceinline Execution::Thread * GetActiveThread()
        {
            return (Execution::Thread *)CpuInstructions::GsGetPointer(offsetof(struct CpuData, ActiveThread));
        }
        static __forceinline Execution::Thread * SetActiveThread(Execution::Thread * const val)
        {
            return (Execution::Thread *)CpuInstructions::GsSetPointer(offsetof(struct CpuData, ActiveThread), (uintptr_t)val);
        }

        static __forceinline Synchronization::Spinlock<> * GetKernelHeapSpinlock()
        {
            return (Synchronization::Spinlock<> *)CpuInstructions::GsGetPointer(offsetof(struct CpuData, HeapSpinlockPointer));
        }
        static __forceinline Synchronization::Spinlock<> * SetKernelHeapSpinlock(Synchronization::Spinlock<> * const val)
        {
            return (Synchronization::Spinlock<> *)CpuInstructions::GsSetPointer(offsetof(struct CpuData, HeapSpinlockPointer), (uintptr_t)val);
        }

        static __forceinline uint16_t GetGdtLength()
        {
            return CpuInstructions::GsGet16(offsetof(struct CpuData, GdtLength));
        }
        static __forceinline uint16_t SetGdtLength(const uint16_t val)
        {
            return CpuInstructions::GsSet16(offsetof(struct CpuData, GdtLength), (uintptr_t)val);
        }

        static __forceinline bool GetX2ApicMode()
        {
            return (bool)CpuInstructions::GsGet8(offsetof(struct CpuData, X2ApicMode));
        }
        static __forceinline bool SetX2ApicMode(const bool val)
        {
            return (bool)CpuInstructions::GsSet8(offsetof(struct CpuData, X2ApicMode), (uint8_t)val);
        }
    };
}}
