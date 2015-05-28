#pragma once

#define __BEELZEBUB__PAGING 1

#define __bland __attribute__((__target__("no-aes,no-mmx,no-pclmul,no-sse,no-sse2,no-sse3,no-sse4,no-sse4a,no-fma4,no-lwp,no-ssse3,no-fancy-math-387,no-ieee-fp,no-recip")))
//  All hail the preprocessor!

//  Some type definitions.

#if   defined(__BEELZEBUB__ARCH_AMD64)
typedef unsigned long long paddr_t; //  Physical address.
typedef unsigned long long vaddr_t; //  Virtual (linear) address.
typedef unsigned long long psize_t; //  Physical size.
typedef unsigned long long vsize_t; //  Virtual (linear) address.
typedef unsigned long long pgind_t; //  Index of a memory page.
#elif defined(__BEELZEBUB__ARCH_IA32PAE)
typedef unsigned long long paddr_t; //  Physical address.
typedef unsigned    int    vaddr_t; //  Virtual (linear) address.
typedef unsigned long long psize_t; //  Physical size.
typedef unsigned    int    vsize_t; //  Virtual (linear) address.
typedef unsigned    int    pgind_t; //  Index of a memory page.
#elif defined(__BEELZEBUB__ARCH_IA32)
typedef unsigned    int    paddr_t; //  Physical address.
typedef unsigned    int    vaddr_t; //  Virtual (linear) address.
typedef unsigned    int    psize_t; //  Physical size.
typedef unsigned    int    vsize_t; //  Virtual (linear) address.
typedef unsigned    int    pgind_t; //  Index of a memory page.
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
