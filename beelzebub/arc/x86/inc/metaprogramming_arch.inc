#define __bland __attribute__((__target__("no-aes,no-mmx,no-pclmul,no-sse,no-sse2,no-sse3,no-sse4,no-sse4a,no-fma4,no-lwp,no-ssse3,no-fancy-math-387,no-ieee-fp,no-recip")))
//  All hail the preprocessor!

//  Some type definitions.

#if   defined(__BEELZEBUB__ARCH_AMD64)
typedef uint64_t paddr_t; //  Physical address.
typedef uint64_t vaddr_t; //  Virtual (linear) address.
typedef uint64_t psize_t; //  Physical size.
typedef uint64_t vsize_t; //  Virtual (linear) address.
typedef uint64_t pgind_t; //  Index of a memory page.
typedef uint64_t  creg_t; //  Control register.

typedef  int64_t ssize_t;

typedef uint64_t volatile int_cookie_t;

#elif defined(__BEELZEBUB__ARCH_IA32)

#if   defined(__BEELZEBUB__ARCH_IA32PAE)
typedef uint64_t paddr_t; //  Physical address.
typedef uint64_t psize_t; //  Physical size.
#else
typedef uint32_t paddr_t; //  Physical address.
typedef uint32_t psize_t; //  Physical size.
#endif

typedef uint32_t vaddr_t; //  Virtual (linear) address.
typedef uint32_t vsize_t; //  Virtual (linear) address.
typedef uint32_t pgind_t; //  Index of a memory page.
typedef uint32_t  creg_t; //  Control register.

typedef  int32_t ssize_t;

typedef uint32_t volatile int_cookie_t;

#endif

typedef uint16_t seg_t; //  Segment register.

#ifdef __GNUC__
typedef union
{
    void * ptr;
    vaddr_t val;
} vaddrptr_t __attribute__((transparent_union));
#endif

typedef             int     comp_t; //  Result of comparison functions.

typedef unsigned     char     byte; //  Uhm...
typedef   signed     char    sbyte; //  Yeah...

#ifndef __cplusplus
typedef char bool;

#define true    ((bool)1)
#define TRUE    ((bool)1)
#define false   ((bool)0)
#define FALSE   ((bool)0)
#endif

/*  Memory barriers and forced ordering  */

#define COMPILER_MEMORY_BARRIER() asm volatile ( "" : : : "memory" )

#define FORCE_ORDER(before, after) asm volatile ( ""                  \
                                                : [out]"=m"(after )   \
                                                : [in ] "m"(before) )
