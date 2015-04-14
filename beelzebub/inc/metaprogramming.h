#pragma once

#include <stdint.h>
#include <arc/metaprogramming.h>

/*	Constants/keywords..?	*/

#define nullptr (0)

/*	This is interesting.	*/

#ifdef __cplusplus
#define __extern extern "C"
#else
#define __extern extern
#endif

/*	This part defines a few function modifiers based on attributes.	*/

#define __forceinline	inline  __attribute__((always_inline))
#define __const			__attribute__((const))
