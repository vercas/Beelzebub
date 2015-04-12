#pragma once

#include <stdint.h>

/*	Constants/keywords..?	*/

#define nullptr (0)

/*	This is interesting.	*/

#ifdef __cplusplus
#define shared extern "C"
#else
#define shared extern
#endif

/*	This part defines a few function modifiers based on attributes.	*/

#define blandfunc __attribute__((__target__("no-aes,no-mmx,no-pclmul,no-sse,no-sse2,no-sse3,no-sse4,no-sse4a,no-fma4,no-lwp,no-ssse3,no-fancy-math-387,no-ieee-fp,no-recip")))

