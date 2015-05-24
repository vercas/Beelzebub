#pragma once

#include <stdint.h>
#include "stddef.h"
#include <arc/metaprogramming.h>

/*  Some macro helpers. */

#define MCATS2(A, B) A ## B
#define MCATS3(A, B, C) A ## B ## C
#define MCATS4(A, B, C, D) A ## B ## C ## D
//  Macro conCATenate Symbols!

/*  Constants/keywords..?   */

#define nullptr (0)

/*  This is interesting.    */

#ifdef __cplusplus
#define __extern extern "C"
#else
#define __extern extern
#endif

/*  This part defines a few function modifiers based on attributes. */

#ifdef __GNUC__
#define __forceinline   inline  __attribute__((always_inline))
#define __const         __attribute__((const))
#define __cold          __attribute__((cold))
#define __hot           __attribute__((hot))
#define __noreturn      __attribute__((noreturn))
#define __used          __attribute__((used))
#define __unused        __attribute__((unused))

#define likely(expr)    (__builtin_expect((expr), 1))
#define unlikely(expr)  (__builtin_expect((expr), 0))
#else
#define likely(expr)    (expr)
#define unlikely(expr)  (expr)
#endif

//  These exist because they are shorter and I can later adapt them for
//  other compilers as well.
