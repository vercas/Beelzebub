#pragma once

#include <memory/paging.hpp>
#include <system/registers.hpp>
#include <system/registers_x86.hpp>

#include <synchronization/spinlock.hpp>
#include <execution/thread.hpp>

#include <system/cpuid.hpp>
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
        Synchronization::Spinlock * HeapSpinlockPointer;
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

        /*  Control  */

        static const bool CanHalt = true;

        static __bland __forceinline void Halt()
        {
            asm volatile ( "hlt \n\t" );
        }

        /*  Caching  */

        static __bland __forceinline void WriteBackAndInvalidateCache()
        {
            asm volatile ( "wbinvd \n\t" : : : "memory" );
        }

        /*  Far memory ops  */

        static __bland __forceinline uint8_t FsGet8(const uintptr_t off)
        {
            uint8_t ret;

            asm volatile ( "movb %%fs:(%1), %0 \n\t"
                         : "=r"(ret)
                         : "r"(off) );

            return ret;
        }
        static __bland __forceinline uint8_t FsSet8(const uintptr_t off, const uint8_t val)
        {
            asm volatile ( "movb %0, %%fs:(%1) \n\t"
                         :
                         : "r"(val), "r"(off) );

            return val;
        }

        static __bland __forceinline uint8_t GsGet8(const uintptr_t off)
        {
            uint8_t ret;

            asm volatile ( "movb %%gs:(%1), %0 \n\t"
                         : "=r"(ret)
                         : "r"(off) );

            return ret;
        }
        static __bland __forceinline uint8_t GsSet8(const uintptr_t off, const uint8_t val)
        {
            asm volatile ( "movb %0, %%gs:(%1) \n\t"
                         :
                         : "r"(val), "r"(off) );

            return val;
        }

        static __bland __forceinline uint16_t FsGet16(const uintptr_t off)
        {
            uint16_t ret;

            asm volatile ( "movw %%fs:(%1), %0 \n\t"
                         : "=r"(ret)
                         : "r"(off) );

            return ret;
        }
        static __bland __forceinline uint16_t FsSet16(const uintptr_t off, const uint16_t val)
        {
            asm volatile ( "movw %0, %%fs:(%1) \n\t"
                         :
                         : "r"(val), "r"(off) );

            return val;
        }

        static __bland __forceinline uint16_t GsGet16(const uintptr_t off)
        {
            uint16_t ret;

            asm volatile ( "movw %%gs:(%1), %0 \n\t"
                         : "=r"(ret)
                         : "r"(off) );

            return ret;
        }
        static __bland __forceinline uint16_t GsSet16(const uintptr_t off, const uint16_t val)
        {
            asm volatile ( "movw %0, %%gs:(%1) \n\t"
                         :
                         : "r"(val), "r"(off) );

            return val;
        }

        static __bland __forceinline uint32_t FsGet32(const uintptr_t off)
        {
            uint32_t ret;

            asm volatile ( "movl %%fs:(%1), %0 \n\t"
                         : "=r"(ret)
                         : "r"(off) );

            return ret;
        }
        static __bland __forceinline uint32_t FsSet32(const uintptr_t off, const uint32_t val)
        {
            asm volatile ( "movl %0, %%fs:(%1) \n\t"
                         :
                         : "r"(val), "r"(off) );

            return val;
        }

        static __bland __forceinline uint32_t GsGet32(const uintptr_t off)
        {
            uint32_t ret;

            asm volatile ( "movl %%gs:(%1), %0 \n\t"
                         : "=r"(ret)
                         : "r"(off) );

            return ret;
        }
        static __bland __forceinline uint32_t GsSet32(const uintptr_t off, const uint32_t val)
        {
            asm volatile ( "movl %0, %%gs:(%1) \n\t"
                         :
                         : "r"(val), "r"(off) );

            return val;
        }

#if   defined(__BEELZEBUB__ARCH_AMD64)
        static __bland __forceinline uint64_t FsGet64(const uintptr_t off)
        {
            uint64_t ret;

            asm volatile ( "movq %%fs:(%1), %0 \n\t"
                         : "=r"(ret)
                         : "r"(off) );

            return ret;
        }
        static __bland __forceinline uint64_t FsSet64(const uintptr_t off, const uint64_t val)
        {
            asm volatile ( "movq %0, %%fs:(%1) \n\t"
                         :
                         : "r"(val), "r"(off) );

            return val;
        }

        static __bland __forceinline uint64_t GsGet64(const uintptr_t off)
        {
            uint64_t ret;

            asm volatile ( "movq %%gs:(%1), %0 \n\t"
                         : "=r"(ret)
                         : "r"(off) );

            return ret;
        }
        static __bland __forceinline uint64_t GsSet64(const uintptr_t off, const uint64_t val)
        {
            asm volatile ( "movq %0, %%gs:(%1) \n\t"
                         :
                         : "r"(val), "r"(off) );

            return val;
        }

#define FsGetPointer(off     )    ((uintptr_t)(FsGet64(off     )))
#define FsSetPointer(off, val)    ((uintptr_t)(FsSet64(off, val)))
#define GsGetPointer(off     )    ((uintptr_t)(GsGet64(off     )))
#define GsSetPointer(off, val)    ((uintptr_t)(GsSet64(off, val)))
#define FsGetSize(off     )       ((   size_t)(FsGet64(off     )))
#define FsSetSize(off, val)       ((   size_t)(FsSet64(off, val)))
#define GsGetSize(off     )       ((   size_t)(GsGet64(off     )))
#define GsSetSize(off, val)       ((   size_t)(GsSet64(off, val)))

#else
        static __bland __forceinline uint64_t FsGet64(const uintptr_t off)
        {
            uint32_t low;
            uint32_t high;

            asm volatile ( "movl %%fs:(%2    ), %0 \n\t"
                           "movl %%fs:(%2 + 4), %1 \n\t"
                         : "=r"(low), "=r"(high)
                         : "r"(off) );

            return ((uint64_t)high << 32) | (uint64_t)low;
        }
        static __bland __forceinline uint64_t FsSet64(const uintptr_t off, const uint64_t val)
        {
            asm volatile ( "movl %0, %%fs:(%2    ) \n\t"
                           "movl %1, %%fs:(%2 + 4) \n\t"
                         : 
                         : "r"((uint32_t)val)
                         , "r"((uint32_t)(val >> 32))
                         , "r"(off) );

            return val;
        }

        static __bland __forceinline uint64_t GsGet64(const uintptr_t off)
        {
            uint32_t low;
            uint32_t high;

            asm volatile ( "movl %%gs:(%2    ), %0 \n\t"
                           "movl %%gs:(%2 + 4), %1 \n\t"
                         : "=r"(low), "=r"(high)
                         : "r"(off) );

            return ((uint64_t)high << 32) | (uint64_t)low;
        }
        static __bland __forceinline uint64_t GsSet64(const uintptr_t off, const uint64_t val)
        {
            asm volatile ( "movl %0, %%gs:(%2    ) \n\t"
                           "movl %1, %%gs:(%2 + 4) \n\t"
                         : 
                         : "r"((uint32_t)val)
                         , "r"((uint32_t)(val >> 32))
                         , "r"(off) );

            return val;
        }

#define FsGetPointer(off     )    ((uintptr_t)(FsGet32(off     )))
#define FsSetPointer(off, val)    ((uintptr_t)(FsSet32(off, val)))
#define GsGetPointer(off     )    ((uintptr_t)(GsGet32(off     )))
#define GsSetPointer(off, val)    ((uintptr_t)(GsSet32(off, val)))
#define FsGetSize(off     )       ((   size_t)(FsGet32(off     )))
#define FsSetSize(off, val)       ((   size_t)(FsSet32(off, val)))
#define GsGetSize(off     )       ((   size_t)(GsGet32(off     )))
#define GsSetSize(off, val)       ((   size_t)(GsSet32(off, val)))

#endif

        static __bland __forceinline uint32_t FarGet32(const uint16_t sel
                                                     , const uintptr_t off)
        {
            uint32_t ret;

            asm volatile ( "push %%fs          \n\t"
                           "mov  %1, %%fs      \n\t"
                           "mov  %%fs:(%2), %0 \n\t"
                           "pop  %%fs          \n\t"
                           : "=r"(ret)
                           : "g"(sel), "r"(off) );

            return ret;
        }

        /*  Segment Registers  */

        REGFUNC3(cs, Cs, seg_t)
        REGFUNC3(ds, Ds, seg_t)
        REGFUNC3(ss, Ss, seg_t)
        REGFUNC3(es, Es, seg_t)
        REGFUNC3(fs, Fs, seg_t)
        REGFUNC3(gs, Gs, seg_t)

        /*  Port I/O  */

        static __bland __forceinline void Out8(const uint16_t port
                                             , const uint8_t value)
        {
            asm volatile ( "outb %1, %0 \n\t"
                         :
                         : "dN" (port), "a" (value) );
        }

        static __bland __forceinline void Out16(const uint16_t port
                                              , const uint16_t value)
        {
            asm volatile ( "outw %1, %0 \n\t"
                         :
                         : "dN" (port), "a" (value) );
        }

        static __bland __forceinline void Out32(const uint16_t port
                                              , const uint32_t value)
        {
            asm volatile ( "outl %1, %0 \n\t"
                         :
                         : "dN" (port), "a" (value) );
        }


        static __bland __forceinline void In8(const uint16_t port, uint8_t & value)
        {
            asm volatile ( "inb %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );
        }

        static __bland __forceinline void In16(const uint16_t port, uint16_t & value)
        {
            asm volatile ( "inw %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );
        }

        static __bland __forceinline void In32(const uint16_t port, uint32_t & value)
        {
            asm volatile ( "inl %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );
        }


        static __bland __forceinline uint8_t In8(const uint16_t port)
        {
            uint8_t value;

            asm volatile ( "inb %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );

            return value;
        }

        static __bland __forceinline uint16_t In16(const uint16_t port)
        {
            uint16_t value;

            asm volatile ( "inw %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );

            return value;
        }

        static __bland __forceinline uint32_t In32(const uint16_t port)
        {
            uint32_t value;

            asm volatile ( "inl %1, %0 \n\t"
                         : "=a" (value)
                         : "dN" (port) );

            return value;
        }

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
            return GsGet32(offsetof(struct CpuData, Index));
        }
        static __bland __forceinline uint32_t SetIndex(const uint32_t val)
        {
            return (uint32_t)GsSet32(offsetof(struct CpuData, Index), val);
        }

        /*static __bland __forceinline uint32_t GetInterruptDisableCount()
        {
            return GsGet32(offsetof(struct CpuData, InterruptDisableCount));
        }
        static __bland __forceinline uint32_t SetInterruptDisableCount(const uint32_t val)
        {
            return (uint32_t)GsSet32(offsetof(struct CpuData, InterruptDisableCount), val);
        }*/

        static __bland __forceinline Execution::Thread * GetActiveThread()
        {
            return (Execution::Thread *)GsGetPointer(offsetof(struct CpuData, ActiveThread));
        }
        static __bland __forceinline Execution::Thread * SetActiveThread(Execution::Thread * const val)
        {
            return (Execution::Thread *)GsSetPointer(offsetof(struct CpuData, ActiveThread), (uintptr_t)val);
        }

        static __bland __forceinline Synchronization::Spinlock * GetKernelHeapSpinlock()
        {
            return (Synchronization::Spinlock *)GsGetPointer(offsetof(struct CpuData, HeapSpinlockPointer));
        }
        static __bland __forceinline Synchronization::Spinlock * SetKernelHeapSpinlock(Synchronization::Spinlock * const val)
        {
            return (Synchronization::Spinlock *)GsSetPointer(offsetof(struct CpuData, HeapSpinlockPointer), (uintptr_t)val);
        }

        static __bland __forceinline vaddr_t GetKernelHeapStart()
        {
            return (vaddr_t)GsGetPointer(offsetof(struct CpuData, KernelHeapStart));
        }
        static __bland __forceinline vaddr_t SetKernelHeapStart(const vaddr_t val)
        {
            return (vaddr_t)GsSetPointer(offsetof(struct CpuData, KernelHeapStart), (uintptr_t)val);
        }

        static __bland __forceinline vaddr_t GetKernelHeapCursor()
        {
            return (vaddr_t)GsGetPointer(offsetof(struct CpuData, KernelHeapCursor));
        }
        static __bland __forceinline vaddr_t SetKernelHeapCursor(const vaddr_t val)
        {
            return (vaddr_t)GsSetPointer(offsetof(struct CpuData, KernelHeapCursor), (uintptr_t)val);
        }

        static __bland __forceinline vaddr_t GetKernelHeapEnd()
        {
            return (vaddr_t)GsGetPointer(offsetof(struct CpuData, KernelHeapEnd));
        }
        static __bland __forceinline vaddr_t SetKernelHeapEnd(const vaddr_t val)
        {
            return (vaddr_t)GsSetPointer(offsetof(struct CpuData, KernelHeapEnd), (uintptr_t)val);
        }

        /*  Interrupts  */

        //static_assert(4 == offsetof(struct CpuData, InterruptDisableCount));
        //  Protecting against accidental changes of the CpuData struct without updating the appropriate functions.

        static __bland __forceinline bool InterruptsEnabled()
        {
            size_t flags;

            asm volatile ( "pushf\n\t"
                           "pop %0\n\t"
                           : "=rm"(flags) );

            return (flags & (size_t)(1 << 9)) != 0;
        }

        static __bland __forceinline void EnableInterrupts()
        {
            asm volatile ( "sti \n\t" : : : "memory" );
            //  This is a memory barrier to prevent the compiler from moving things around it.
        }

        static __bland __forceinline void DisableInterrupts()
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
        static __bland __forceinline int_cookie_t PushDisableInterrupts()
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
        static __bland __forceinline bool RestoreInterruptState(const int_cookie_t cookie)
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

        static __bland __forceinline void LIDT(const uintptr_t base, const uint16_t size)
        {
            struct
            {
                uint16_t length;
                uintptr_t base;
            } __packed IDTR;

            IDTR.length = size;
            IDTR.base = base;

            asm volatile ( "lidt (%0)\n\t" : : "p"(&IDTR) );
        }
    };
}}
