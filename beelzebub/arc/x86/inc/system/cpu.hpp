#pragma once

#include <memory/paging.hpp>
#include <system/registers.hpp>
#include <system/registers_x86.hpp>
#include <system/cpu_instructions.hpp>

#include <synchronization/spinlock.hpp>
#include <execution/thread.hpp>

#include <system/cpuid.hpp>
#include <synchronization/atomic.hpp>
#include <metaprogramming.h>

#define REGFUNC1(regl, regu, type)                                   \
static __bland __forceinline type MCATS2(Get, regu)()                \
{                                                                    \
    type ret;                                                        \
                                                                     \
    asm volatile ( "mov %%" #regl ", %0\n\t"                         \
                 : "=r"(ret) );                                      \
                                                                     \
    return ret;                                                      \
}                                                                    \
static __bland __forceinline void MCATS2(Set, regu)(const type val)  \
{                                                                    \
    asm volatile ( "mov %0, %%" #regl "\n\t"                         \
                 : : "r"(val) );                                     \
}

#define REGFUNC2(regl, regu, type, type2)                            \
static __bland __forceinline type2 MCATS2(Get, regu)()               \
{                                                                    \
    type ret;                                                        \
                                                                     \
    asm volatile ( "mov %%" #regl ", %0\n\t"                         \
                 : "=r"(ret) );                                      \
                                                                     \
    return type2(ret);                                               \
}                                                                    \
static __bland __forceinline void MCATS2(Set, regu)(const type val)  \
{                                                                    \
    asm volatile ( "mov %0, %%" #regl "\n\t"                         \
                 : : "r"(val) );                                     \
}                                                                    \
static __bland __forceinline void MCATS2(Set, regu)(const type2 val) \
{                                                                    \
    type innerVal = val.Value;                                       \
                                                                     \
    asm volatile ( "mov %0, %%" #regl "\n\t"                         \
                 : : "r"(innerVal) );                                \
}

#define REGFUNC3(regl, regu, type)                                   \
static __bland __forceinline type MCATS2(Get, regu)()                \
{                                                                    \
    type ret;                                                        \
                                                                     \
    asm volatile ( "mov %%" #regl ", %0\n\t"                         \
                 : "=r"(ret) );                                      \
                                                                     \
    return ret;                                                      \
}

#define MSRFUNC1(name, prettyName, type)                                  \
static __bland __forceinline type MCATS2(Get, prettyName)()               \
{                                                                         \
    uint64_t temp = 0;                                                    \
    ReadMsr(Msr::name, temp);                                             \
    return type(temp);                                                    \
}                                                                         \
static __bland __forceinline void MCATS2(Set, prettyName)(const type val) \
{                                                                         \
    WriteMsr(Msr::name, val.Value);                                       \
}

namespace Beelzebub { namespace System
{
    /**
     *  The data available to an individual CPU core.
     */
    struct CpuData
    {
        uint32_t Index;
        uint32_t InterruptDisableCount; /* PADDING */
        Execution::Thread * ActiveThread;
        Synchronization::spinlock_t HeapSpinlock;
        Synchronization::Spinlock<> * HeapSpinlockPointer;

        vaddr_t KernelHeapStart;
        vaddr_t KernelHeapCursor;
        vaddr_t KernelHeapEnd;
    } __packed;

    /**
     *  Represents a processing unit of the system.
     */
    class Cpu
    {
    public:

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

        /*  MSRs  */

        static __bland __forceinline MsrValue ReadMsr(const Msr reg)
        {
            uint32_t a, d;

            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            return {{d, a}};
        }

        static __bland __forceinline uint64_t ReadMsr64(const Msr reg)
        {
            uint32_t a, d;

            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            return (uint64_t)a | ((uint64_t)d << 32);
        }

        static __bland __forceinline void ReadMsr(const Msr reg, uint32_t & a, uint32_t & d)
        {
            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );
        }

        static __bland __forceinline void ReadMsr(const Msr reg, uint64_t & val)
        {
            uint32_t a, d;

            asm volatile ( "rdmsr \n\t"
                         : "=a" (a), "=d" (d)
                         : "c" (reg) );

            val = (uint64_t)a | ((uint64_t)d << 32);
        }

        static __bland __forceinline void WriteMsr(const Msr reg, const MsrValue val)
        {
            asm volatile ( "wrmsr \n\t"
                         : 
                         : "c" (reg), "a" (val.Dwords.Low), "d" (val.Dwords.High) );
        }

        static __bland __forceinline void WriteMsr(const Msr reg, const uint32_t a, const uint32_t d)
        {
            asm volatile ( "wrmsr \n\t"
                         : 
                         : "c" (reg), "a" (a), "d" (d) );
        }

        static __bland __forceinline void WriteMsr(const Msr reg, const uint64_t val)
        {
            register uint32_t a asm("eax") = (uint32_t)val;
            register uint32_t d asm("edx") = (uint32_t)(val >> 32);

            asm volatile ( "wrmsr \n\t"
                         : 
                         : "c" (reg), "a" (a), "d" (d) );
        }

        MSRFUNC1(IA32_EFER, EFER, Ia32Efer)

        /*  Shortcuts  */

        static __bland __forceinline void EnableNxBit()
        {
            const Msr reg = Msr::IA32_EFER;

            asm volatile ( "rdmsr           \n\t"
                           "or $2048, %%eax \n\t" //  That be bit 11.
                           "wrmsr           \n\t"
                         :
                         : "c" (reg)
                         : "eax", "edx" );
        }

        static __bland size_t ComputeIndex();

        /*  CPU-specific data  */

        static const size_t CpuDataSize = sizeof(CpuData);

        static __bland __forceinline uint32_t GetIndex()
        {
            return CpuInstructions::GsGet32(offsetof(struct CpuData, Index));
        }
        static __bland __forceinline uint32_t SetIndex(const uint32_t val)
        {
            return (uint32_t)CpuInstructions::GsSet32(offsetof(struct CpuData, Index), val);
        }

        /*static __bland __forceinline uint32_t GetInterruptDisableCount()
        {
            return CpuInstructions::GsGet32(offsetof(struct CpuData, InterruptDisableCount));
        }
        static __bland __forceinline uint32_t SetInterruptDisableCount(const uint32_t val)
        {
            return (uint32_t)CpuInstructions::GsSet32(offsetof(struct CpuData, InterruptDisableCount), val);
        }*/

        static __bland __forceinline Execution::Thread * GetActiveThread()
        {
            return (Execution::Thread *)CpuInstructions::GsGetPointer(offsetof(struct CpuData, ActiveThread));
        }
        static __bland __forceinline Execution::Thread * SetActiveThread(Execution::Thread * const val)
        {
            return (Execution::Thread *)CpuInstructions::GsSetPointer(offsetof(struct CpuData, ActiveThread), (uintptr_t)val);
        }

        static __bland __forceinline Synchronization::Spinlock<> * GetKernelHeapSpinlock()
        {
            return (Synchronization::Spinlock<> *)CpuInstructions::GsGetPointer(offsetof(struct CpuData, HeapSpinlockPointer));
        }
        static __bland __forceinline Synchronization::Spinlock<> * SetKernelHeapSpinlock(Synchronization::Spinlock<> * const val)
        {
            return (Synchronization::Spinlock<> *)CpuInstructions::GsSetPointer(offsetof(struct CpuData, HeapSpinlockPointer), (uintptr_t)val);
        }

        static __bland __forceinline vaddr_t GetKernelHeapStart()
        {
            return (vaddr_t)CpuInstructions::GsGetPointer(offsetof(struct CpuData, KernelHeapStart));
        }
        static __bland __forceinline vaddr_t SetKernelHeapStart(const vaddr_t val)
        {
            return (vaddr_t)CpuInstructions::GsSetPointer(offsetof(struct CpuData, KernelHeapStart), (uintptr_t)val);
        }

        static __bland __forceinline vaddr_t GetKernelHeapCursor()
        {
            return (vaddr_t)CpuInstructions::GsGetPointer(offsetof(struct CpuData, KernelHeapCursor));
        }
        static __bland __forceinline vaddr_t SetKernelHeapCursor(const vaddr_t val)
        {
            return (vaddr_t)CpuInstructions::GsSetPointer(offsetof(struct CpuData, KernelHeapCursor), (uintptr_t)val);
        }

        static __bland __forceinline vaddr_t GetKernelHeapEnd()
        {
            return (vaddr_t)CpuInstructions::GsGetPointer(offsetof(struct CpuData, KernelHeapEnd));
        }
        static __bland __forceinline vaddr_t SetKernelHeapEnd(const vaddr_t val)
        {
            return (vaddr_t)CpuInstructions::GsSetPointer(offsetof(struct CpuData, KernelHeapEnd), (uintptr_t)val);
        }
    };
}}
