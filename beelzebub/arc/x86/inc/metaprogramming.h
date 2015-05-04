#pragma once

#define __BEELZEBUB__PAGING 1

#define __bland __attribute__((__target__("no-aes,no-mmx,no-pclmul,no-sse,no-sse2,no-sse3,no-sse4,no-sse4a,no-fma4,no-lwp,no-ssse3,no-fancy-math-387,no-ieee-fp,no-recip")))

//	Some type definitions.

typedef unsigned long long paddr_t;	//	Physical address.
typedef unsigned long long vaddr_t;	//	Virtual (linear) address.
typedef unsigned long long psize_t;	//	Physical size.
typedef unsigned long long vsize_t;	//	Virtual (linear) address.
typedef unsigned long long pgind_t;	//	Index of a memory page.

typedef uint8_t            byte;	//	Uhm...
