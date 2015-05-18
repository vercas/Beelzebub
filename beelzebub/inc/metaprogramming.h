#pragma once

#include <stdint.h>
#include "stddef.h"
#include <arc/metaprogramming.h>

/*	Some macro helpers.	*/

#define MCATS2(A, B) A ## B
#define MCATS3(A, B, C) A ## B ## C
#define MCATS4(A, B, C, D) A ## B ## C ## D
//	Macro conCATenate Symbols!

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
