#pragma once

#include <metaprogramming.h>

namespace Beelzebub { namespace System
{
    /**
     *  Represents a processing unit of the system.
     */
    class CpuInstructions
    {
        /*  Constructor(s)  */

    protected:
        CpuInstructions() = default;

    public:
        CpuInstructions(CpuInstructions const &) = delete;
        CpuInstructions & operator =(CpuInstructions const &) = delete;

        /*  Control  */

        static bool const CanHalt = true;

        static __bland __forceinline void Halt()
        {
            asm volatile ( "hlt \n\t" : : : "memory" );
        }

        static __bland __forceinline void DoNothing()
        {
            asm volatile ( "pause \n\t" : : : "memory" );
        }

        /*  Caching  */

        static __bland __forceinline void WriteBackAndInvalidateCache()
        {
            asm volatile ( "wbinvd \n\t" : : : "memory" );
        }

        /*  Profiling  */

#if   defined(__BEELZEBUB__ARCH_AMD64)
        static __bland __forceinline uint64_t Rdtsc()
        {
            uint64_t low, high;

            asm volatile ( "rdtsc \n\t" : "=a"(low), "=d"(high) );

            return (high << 32) | low;
        }
#else
        static __bland __forceinline uint64_t Rdtsc()
        {
            uint64_t res;

            asm volatile ( "rdtsc \n\t" : "=A"(res) );

            return res;
        }
#endif

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
                         : "r"(val), "r"(off)
                         : "memory" );

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
                         : "r"(val), "r"(off)
                         : "memory" );

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
                         : "r"(val), "r"(off)
                         : "memory" );

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
                         : "r"(val), "r"(off)
                         : "memory" );

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
                         : "r"(val), "r"(off)
                         : "memory" );

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
                         : "r"(val), "r"(off)
                         : "memory" );

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
                         : "r"(val), "r"(off)
                         : "memory" );

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
                         : "r"(val), "r"(off)
                         : "memory" );

            return val;
        }

        static __bland __forceinline uintptr_t FsGetPointer(uintptr_t const off                     ) { return (uintptr_t)FsGet64(off     ); }
        static __bland __forceinline uintptr_t FsSetPointer(uintptr_t const off, uintptr_t const val) { return (uintptr_t)FsSet64(off, val); }
        static __bland __forceinline uintptr_t GsGetPointer(uintptr_t const off                     ) { return (uintptr_t)GsGet64(off     ); }
        static __bland __forceinline uintptr_t GsSetPointer(uintptr_t const off, uintptr_t const val) { return (uintptr_t)GsSet64(off, val); }
        static __bland __forceinline    size_t FsGetSize(uintptr_t const off                     )    { return (   size_t)FsGet64(off     ); }
        static __bland __forceinline    size_t FsSetSize(uintptr_t const off,    size_t const val)    { return (   size_t)FsSet64(off, val); }
        static __bland __forceinline    size_t GsGetSize(uintptr_t const off                     )    { return (   size_t)GsGet64(off     ); }
        static __bland __forceinline    size_t GsSetSize(uintptr_t const off,    size_t const val)    { return (   size_t)GsSet64(off, val); }

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
                         , "r"(off)
                         : "memory" );

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
                         , "r"(off)
                         : "memory" );

            return val;
        }

        static __bland __forceinline uintptr_t FsGetPointer(uintptr_t const off                     ) { return (uintptr_t)FsGet32(off     ); }
        static __bland __forceinline uintptr_t FsSetPointer(uintptr_t const off, uintptr_t const val) { return (uintptr_t)FsSet32(off, val); }
        static __bland __forceinline uintptr_t GsGetPointer(uintptr_t const off                     ) { return (uintptr_t)GsGet32(off     ); }
        static __bland __forceinline uintptr_t GsSetPointer(uintptr_t const off, uintptr_t const val) { return (uintptr_t)GsSet32(off, val); }
        static __bland __forceinline    size_t FsGetSize(uintptr_t const off                     )    { return (   size_t)FsGet32(off     ); }
        static __bland __forceinline    size_t FsSetSize(uintptr_t const off,    size_t const val)    { return (   size_t)FsSet32(off, val); }
        static __bland __forceinline    size_t GsGetSize(uintptr_t const off                     )    { return (   size_t)GsGet32(off     ); }
        static __bland __forceinline    size_t GsSetSize(uintptr_t const off,    size_t const val)    { return (   size_t)GsSet32(off, val); }

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

        /*  Interrupts  */

        static __bland __forceinline void LIDT(const uintptr_t base, const uint16_t size)
        {
            struct
            {
                uint16_t length;
                uintptr_t base;
            } __packed IDTR;

            IDTR.length = size;
            IDTR.base = base;

            asm volatile ( "lidt %0 \n\t" : : "m"(IDTR) );
        }
    };
}}
