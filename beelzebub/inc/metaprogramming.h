#pragma once

#include "stdint.h"
#include "stddef.h"
#include <metaprogramming_arc.h>
#include <cpp_support.h>

/*  Some macro helpers. */

#define GET_ARG1(_1) _1
#define GET_ARG2(_2, _1) _1
#define GET_ARG3(_3, _2, _1) _1
#define GET_ARG4(_4, _3, _2, _1) _1

#define GET_MACRO2(_1, _2, NAME, ...) NAME
#define GET_MACRO6(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
//	Got this from http://stackoverflow.com/a/11763277

#define MCATS1(A) A
#define MCATS2(A, B) A##B
#define MCATS3(A, B, C) A##B##C
#define MCATS4(A, B, C, D) A##B##C##D
#define MCATS5(A, B, C, D, E) A##B##C##D##E
#define MCATS6(A, B, C, D, E, F) A##B##C##D##E##F
#define MCATS(...) GET_MACRO6(__VA_ARGS__, MCATS6, MCATS5, MCATS4, MCATS3, \
                                           MCATS2, MCATS1)(__VA_ARGS__)
//  Macro conCATenate Symbols!

#define __ARG_N(                              \
     __1,__2,__3,__4,__5,__6,__7,__8,__9,_10, \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,_64,_65,_66,_67,_68,_69, N, ...) N
#define __RSEQ_N()                  \
     69,68,67,66,65,64,63,62,61,60, \
     59,58,57,56,55,54,53,52,51,50, \
     49,48,47,46,45,44,43,42,41,40, \
     39,38,37,36,35,34,33,32,31,30, \
     29,28,27,26,25,24,23,22,21,20, \
     19,18,17,16,15,14,13,12,11,10, \
      9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define __NARG_I_(...) __ARG_N(__VA_ARGS__)
#define __NARG__(...)  __NARG_I_(__VA_ARGS__,__RSEQ_N())

// general definition for any function name
#define _VADEF_(name, n) name##n
#define _VADEF(name, n) _VADEF_(name, n)
#define VADEF(func, ...) _VADEF(func, __NARG__(__VA_ARGS__)) (__VA_ARGS__)
//	Got this from http://stackoverflow.com/a/26408195

/*  Constants/keywords..?   */

#ifdef __cplusplus
#define nullpaddr ((paddr_t)(uintptr_t)nullptr)
#define nullvaddr ((vaddr_t)(uintptr_t)nullptr)
#else
#define nullptr   ((void *) 0)
#define nullpaddr ((paddr_t)0)
#define nullvaddr ((vaddr_t)0)
#endif

/*  This is interesting.    */

#ifdef __cplusplus
#define __extern extern "C"
#else
#define __extern extern
#endif

/*  This part defines a few function modifiers based on attributes. */

#ifdef __GNUC__

#define __forceinline      inline  __attribute__((__always_inline__))
#define __noinline         __attribute__((__noinline__))

#define __const            __attribute__((__const__))
#define __cold             __attribute__((__cold__))
#define __hot              __attribute__((__hot__))
#define __noreturn         __attribute__((__noreturn__))
#define __used             __attribute__((__used__))
#define __unused           __attribute__((__unused__))
#define __must_check       __attribute__((__warn_unused_result__))
#define __restrict         __restrict__

#define likely(expr)       (__builtin_expect((expr), 1))
#define unlikely(expr)     (__builtin_expect((expr), 0))

#define __unreachable_code __builtin_unreachable()
#define __prefetch         __builtin_prefetch

#define __packed           __attribute__((__packed__))
#define __alignof(T)       __alignof__(T)

#define __build_data       __attribute__((__section__(".build_data")))

#define __fastcall         __attribute__((__fastcall__))
//  To be used on IA-32 on *some* functions.

#else

#define __forceinline      inline
#define __noinline

#define __const
#define __cold
#define __hot
#define __noreturn
#define __used
#define __unused
#define __must_check

#define likely(expr)       (expr)
#define unlikely(expr)     (expr)

#define __unreachable_code do { } while (false)
#define __prefetch(...)    do { } while (false)

#define __packed

#define __build_data

#endif

//  These exist because they are shorter and I can later adapt them for
//  other compilers as well.


#ifdef __cplusplus

#define ENUMOPS2(T, U)                                                     \
inline  T   operator ~  (T   a     ) { return (T  )(~((U  )(a))          ); } \
inline  T   operator |  (T   a, T b) { return (T  )(  (U  )(a) |  (U )(b)); } \
inline  T   operator &  (T   a, T b) { return (T  )(  (U  )(a) &  (U )(b)); } \
inline  T   operator ^  (T   a, T b) { return (T  )(  (U  )(a) ^  (U )(b)); } \
inline  T & operator |= (T & a, T b) { return (T &)(  (U &)(a) |= (U )(b)); } \
inline  T & operator &= (T & a, T b) { return (T &)(  (U &)(a) &= (U )(b)); } \
inline  T & operator ^= (T & a, T b) { return (T &)(  (U &)(a) ^= (U )(b)); } \
inline bool operator == (U   a, T b) { return               a  == (U )(b);  } \
inline bool operator != (U   a, T b) { return               a  != (U )(b);  } \
inline bool operator == (T   a, U b) { return         (U  )(a) ==      b ;  } \
inline bool operator != (T   a, U b) { return         (U  )(a) !=      b ;  }

#define ENUMOPS1(T) ENUMOPS2(T, __underlying_type(T))
//	All nice and dandy, but it uses a GCC extension for type traits because
//   I can't include the type_traits.h header!

#define ENUMOPS(...) GET_MACRO2(__VA_ARGS__, ENUMOPS2, ENUMOPS1)(__VA_ARGS__)

//  Why? For the glory of C++, of course.
#endif

/*	Some type aliases...  */

typedef  int8_t    Int8;
typedef  int16_t   Int16;
typedef  int32_t   Int32;
typedef  int64_t   Int64;

typedef  int8_t   SInt8;
typedef  int16_t  SInt16;
typedef  int32_t  SInt32;
typedef  int64_t  SInt64;

typedef uint8_t   UInt8;
typedef uint16_t  UInt16;
typedef uint32_t  UInt32;
typedef uint64_t  UInt64;

typedef  intptr_t  IntPtr;
typedef uintptr_t UIntPtr;

//	I think these names are great... I just like to have 'em here.
