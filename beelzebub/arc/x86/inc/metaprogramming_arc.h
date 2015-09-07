#pragma once

#define __BEELZEBUB__PAGING 1

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

typedef uint64_t uintptr_t;
typedef  int64_t  intptr_t;
//typedef   signed long long ptrdiff_t;

typedef uint64_t int_cookie_t;

#define KERNEL_CODE_SEPARATOR ((uintptr_t)0xFFFFFF8000000000ULL)

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

typedef uint32_t uintptr_t;
typedef  int32_t  intptr_t;
//typedef   signed    int    ptrdiff_t;

typedef uint32_t int_cookie_t;

#define KERNEL_CODE_SEPARATOR ((uintptr_t)0xC0000000U)

#endif

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
