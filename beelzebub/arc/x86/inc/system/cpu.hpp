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

#include <execution/thread.hpp>
#include <exceptions.hpp>

#include <timer.hpp>
#include <mailbox.hpp>

#include <synchronization/atomic.hpp>
#include <synchronization/spinlock.hpp>

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
    //  A bit of configuration:
    enum StackSizes : size_t
    {
          PageFaultStackSize = 1 * PageSize,
        DoubleFaultStackSize = 1 * PageSize,
                CpuStackSize = 3 * PageSize,
    };

    typedef uint16_t   seg_t; //  Segment register.

    /**
     *  The data available to an individual CPU core.
     */
    struct CpuData : public CpuDataBase
    {
        paddr_t LastAlienPml4;
        //  Used to invalidate TLBs when dealing with alien mappings, smartly.

        Domain * DomainDescriptor = nullptr;
        Tss EmbeddedTss;

        ExceptionContext * XContext = nullptr;
        Exception X;

        uint16_t GdtLength;
        uint16_t TssSegment;
        bool X2ApicMode = false;

        Execution::Thread * LastExtendedStateThread = nullptr;

        uint_fast16_t TimersCount = 0;
        TimerEntry Timers[Timer::Count];

#if defined(__BEELZEBUB_SETTINGS_SMP)
        MailboxEntryBase * MailHead = nullptr, * MailTail = nullptr;
        Synchronization::Spinlock<> MailLock {};
        uint64_t MailGeneration = 0;
#endif
    };

    /**
     *  Represents a processing unit of the system.
     */
    class Cpu
    {
    public:
        /*  Data  */

#ifdef __SEG_GS_CPP
        static constexpr CpuData __seg_gs * const Data = nullptr;
#endif

        /*  Constructor(s)  */

    protected:
        Cpu() = default;

    public:
        Cpu(Cpu const &) = delete;
        Cpu & operator =(Cpu const &) = delete;

        /*  Properties  */

#if   defined(__BEELZEBUB_SETTINGS_NO_SMP)
        static constexpr size_t const Count = 1;
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
        REGFUNC2(cr4, Cr4, creg_t, Beelzebub::System::Cr4)

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

        /*  Write protection  */

        static inline bool PushDisableWriteProtect()
        {
            bool ret;

            auto cr0 = GetCr0();
            ret = cr0.GetWriteProtect();
            SetCr0(cr0.SetWriteProtect(false));

            return ret;
        }

        static inline bool PushEnableWriteProtect()
        {
            bool ret;

            auto cr0 = GetCr0();
            ret = cr0.GetWriteProtect();
            SetCr0(cr0.SetWriteProtect(true));

            return ret;
        }

        static inline bool RestoreWriteProtect(bool const val)
        {
            SetCr0(GetCr0().SetWriteProtect(val));

            return val;
        }

        /*  CPU-specific data  */

#ifdef __SEG_GS_CPP
        static __forceinline CpuData * GetData()
        {
            return Data->SelfPointer;
        }

        static __forceinline Execution::Thread * GetThread()
        {
            return &(Data->ActiveThread);
        }

        static __forceinline Execution::Thread * SetThread(Execution::Thread const * val)
        {
            return Data->ActiveThread = val;
        }

        static __forceinline Execution::Process * GetProcess()
        {
            return &(Data->ActiveProcess);
        }

        static __forceinline Execution::Process * SetProcess(Execution::Process const * val)
        {
            return Data->ActiveProcess = val;
        }
#else
        static __forceinline CpuData * GetData()
        {
            return reinterpret_cast<CpuData *>(CpuInstructions::GsGetPointer(
                offsetof(struct CpuData, SelfPointer)
            ));
        }

        static __forceinline Execution::Thread * GetThread()
        {
            return reinterpret_cast<Execution::Thread *>(CpuInstructions::GsGetPointer(
                offsetof(struct CpuData, ActiveThread)
            ));
        }

        static __forceinline Execution::Thread * SetThread(Execution::Thread const * val)
        {
            return reinterpret_cast<Execution::Thread *>(CpuInstructions::GsSetPointer(
                offsetof(struct CpuData, ActiveThread),
                reinterpret_cast<uintptr_t>(val)
            ));
        }

        static __forceinline Execution::Process * GetProcess()
        {
            return reinterpret_cast<Execution::Process *>(CpuInstructions::GsGetPointer(
                offsetof(struct CpuData, ActiveProcess)
            ));
        }

        static __forceinline Execution::Process * SetProcess(Execution::Process const * val)
        {
            return reinterpret_cast<Execution::Process *>(CpuInstructions::GsSetPointer(
                offsetof(struct CpuData, ActiveProcess),
                reinterpret_cast<uintptr_t>(val)
            ));
        }
#endif
    };

    /***************************
        Write-Protect Guards
    ***************************/

    /// <summary>Guards a scope from write protection.</summary>
    template<bool en = false>
    struct WriteProtectGuard;

    /// <summary>Guards a scope by disabling write protection.</summary>
    template<>
    struct WriteProtectGuard<false>
    {
        /*  Constructor(s)  */

        inline WriteProtectGuard() : Cookie(Cpu::PushDisableWriteProtect()) { }

        WriteProtectGuard(WriteProtectGuard const &) = delete;
        WriteProtectGuard(WriteProtectGuard && other) = delete;
        WriteProtectGuard & operator =(WriteProtectGuard const &) = delete;
        WriteProtectGuard & operator =(WriteProtectGuard &&) = delete;

        /*  Destructor  */

        inline ~WriteProtectGuard()
        {
            Cpu::RestoreWriteProtect(this->Cookie);
        }

    private:
        /*  Field(s)  */

        bool const Cookie;
    };

    /// <summary>Guards a scope by enabling write protection.</summary>
    template<>
    struct WriteProtectGuard<true>
    {
        /*  Constructor(s)  */

        inline WriteProtectGuard() : Cookie(Cpu::PushEnableWriteProtect()) { }

        WriteProtectGuard(WriteProtectGuard const &) = delete;
        WriteProtectGuard(WriteProtectGuard && other) = delete;
        WriteProtectGuard & operator =(WriteProtectGuard const &) = delete;
        WriteProtectGuard & operator =(WriteProtectGuard &&) = delete;

        /*  Destructor  */

        inline ~WriteProtectGuard()
        {
            Cpu::RestoreWriteProtect(this->Cookie);
        }

    private:
        /*  Field(s)  */

        bool const Cookie;
    };

    #define withWriteProtect(val) with(Beelzebub::System::WriteProtectGuard<val> MCATS(_wp_guard, __LINE__))
}}
