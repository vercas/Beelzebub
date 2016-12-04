/*
    Copyright (c) 2015 Alexandru-Mihai Maftei

    Permission to use, copy, modify, and/or distribute this software for any purpose
    with or without fee is hereby granted, provided that the above copyright notice
    and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
    REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
    FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
    INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
    OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
    THIS SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.

    ---

    Some code here is retrieved or inspired from external sources. Those are
    documented.
*/

#pragma once

#include "stdint.h"
#include "stddef.h"

#ifndef __cplusplus
#include "stdbool.h"
#endif

#if defined(__BEELZEBUB_KERNEL) || defined(__BEELZEBUB_KERNEL_MODULE)
    #define __BEELZEBUB__IN_KERNEL
#endif

#if   defined(__BEELZEBUB__IN_KERNEL) || defined(__BEELZEBUB_STATIC_LIBRARY) || defined(__BEELZEBUB_DYNAMIC_LIBRARY)
#include <cpp_support.h>
#endif

#include <metaprogramming.aux.inc>

/*  Some macro helpers. */

#define GET_ARG1(_1) _1
#define GET_ARG2(_2, _1) _1
#define GET_ARG3(_3, _2, _1) _1
#define GET_ARG4(_4, _3, _2, _1) _1

#define GET_MACRO2(_1, _2, NAME, ...) NAME
#define GET_MACRO3(_1, _2, _3, NAME, ...) NAME
#define GET_MACRO4(_1, _2, _3, _4, NAME, ...) NAME
#define GET_MACRO6(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define GET_MACRO10( _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, NAME, ...) NAME
#define GET_MACRO20( _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10 \
                  , _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, NAME, ...) NAME
#define GET_MACRO100( _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10 \
                   , _11, _12, _13, _14, _15, _16, _17, _18, _19, _20 \
                   , _21, _22, _23, _24, _25, _26, _27, _28, _29, _30 \
                   , _31, _32, _33, _34, _35, _36, _37, _38, _39, _40 \
                   , _41, _42, _43, _44, _45, _46, _47, _48, _49, _50 \
                   , _51, _52, _53, _54, _55, _56, _57, _58, _59, _60 \
                   , _61, _62, _63, _64, _65, _66, _67, _68, _69, _70 \
                   , _71, _72, _73, _74, _75, _76, _77, _78, _79, _80 \
                   , _81, _82, _83, _84, _85, _86, _87, _88, _89, _90 \
                   , _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, NAME, ...) NAME
//  Got this from http://stackoverflow.com/a/11763277

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
//  Got this from http://stackoverflow.com/a/26408195

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
#define __noclone          __attribute__((__noclone__))

#define __const            __attribute__((__const__))
#define __cold             __attribute__((__cold__))
#define __hot              __attribute__((__hot__))
#define __unoptimized      __attribute__((__optimize__(0)))
#define __noreturn         __attribute__((__noreturn__))
#define __returns_twice    __attribute__((__returns_twice__))
#define __used             __attribute__((__used__))
#define __unused           __attribute__((__unused__))
#define __weak             __attribute__((__weak__))
#define __alias(sym)       __attribute__((__alias__(#sym), __weak__, __used__))
#define __must_check       __attribute__((__warn_unused_result__))
#define __restrict         __restrict__
#define __nonnull(...)     __attribute__((__nonnull__(__VA_ARGS__)))
#define __returns_nonnull  __attribute__((__returns_nonnull__))
#define __malloc           __attribute__((__malloc__))

#define likely(expr)       (__builtin_expect((expr), 1))
#define unlikely(expr)     (__builtin_expect((expr), 0))
#define ctconst(val)       (__builtin_constant_p((val)))

#define __unreachable_code __builtin_unreachable()
#define __prefetch         __builtin_prefetch

#define __packed           __attribute__((__packed__))
#define __aligned(n)       __attribute__((__aligned__(n)))
#define __aligned_nat      __attribute__((__aligned__))
#define __alignof(T)       __alignof__(T)

#define __section(name)    __attribute__((__section__("." #name)))
#define __build_data       __attribute__((__section__(".build_data")))

#define __fastcall         __attribute__((__fastcall__))
//  To be used on IA-32 on *some* functions.

#define __startup          __attribute__((__section__(".text.startup"), __cold__, __noinline__, __optimize__("Os")))
#define __userland         __attribute__((__section__(".text.userland"), __used__))

#define __shared            __extern __attribute__((__used__, __optimize__(3), __visibility__("default"), __externally_visible__, __noinline__))
#define __shared_inline     __extern __attribute__((__used__, __optimize__(3), __visibility__("default"), __externally_visible__))
#define __shared_cpp        __attribute__((__used__, __optimize__(3), __visibility__("default"), __externally_visible__, __noinline__))
#define __shared_cpp_inline __attribute__((__used__, __optimize__(3), __visibility__("default"), __externally_visible__))

#define __public           __attribute__((__visibility__("default")))
#define __protected        __attribute__((__visibility__("protected")))
#define __internal         __attribute__((__visibility__("internal")))

#else

#define __forceinline      inline
#define __noinline  
#define __noclone  

#define __const  
#define __cold  
#define __hot  
#define __unoptimized  
#define __noreturn  
#define __returns_twice  
#define __used  
#define __unused  
#define __weak  
#define __alias(sym)  
#define __must_check
#define __restrict
#define __nonnull(...)  
#define __returns_nonnull  
#define __malloc  

#define likely(expr)       (expr)
#define unlikely(expr)     (expr)
#define ctconst(val)       (false)

#define __unreachable_code do { } while (false)
#define __prefetch(...)  

#define __packed  
#define __aligned(n)  
#define __aligned_nat  

#define __section(name)  
#define __build_data  

#define __startup  
#define __userland  

#define __shared           __extern 

#define __public  
#define __protected  
#define __internal  

#endif

#ifndef __fastcall_ia32
#define __fastcall_ia32  
#endif

//  These exist because they are shorter and I can later adapt them for
//  other compilers as well.


#ifdef __cplusplus

#define ENUMOPS_LITE2(T, U)                                                   \
inline bool operator == (U   a, T b) { return               a  == (U )(b);  } \
inline bool operator != (U   a, T b) { return               a  != (U )(b);  } \
inline bool operator == (T   a, U b) { return         (U  )(a) ==      b ;  } \
inline bool operator != (T   a, U b) { return         (U  )(a) !=      b ;  }

#define ENUMOPS2(T, U)                                                        \
inline  T   operator ~  (T   a     ) { return (T  )(~((U  )(a))          ); } \
inline  T   operator |  (T   a, T b) { return (T  )(  (U  )(a) |  (U )(b)); } \
inline  T   operator &  (T   a, T b) { return (T  )(  (U  )(a) &  (U )(b)); } \
inline  T   operator &  (T   a, U b) { return (T  )(  (U  )(a) &       b ); } \
inline  T   operator ^  (T   a, T b) { return (T  )(  (U  )(a) ^  (U )(b)); } \
inline  T & operator |= (T & a, T b) { return (T &)(  (U &)(a) |= (U )(b)); } \
inline  T & operator &= (T & a, T b) { return (T &)(  (U &)(a) &= (U )(b)); } \
inline  T & operator ^= (T & a, T b) { return (T &)(  (U &)(a) ^= (U )(b)); } \
ENUMOPS_LITE2(T, U)

#define ENUMOPS_LITE1(T) ENUMOPS_LITE2(T, __underlying_type(T))
#define ENUMOPS1(T) ENUMOPS2(T, __underlying_type(T))
//  All nice and dandy, but it uses a GCC extension for type traits because
//  the type_traits.h header is unavailable.

#define ENUMOPS_LITE(...) GET_MACRO2(__VA_ARGS__, ENUMOPS_LITE2, ENUMOPS_LITE1)(__VA_ARGS__)
#define ENUMOPS(...) GET_MACRO2(__VA_ARGS__, ENUMOPS2, ENUMOPS1)(__VA_ARGS__)

//  Why? For the glory of C++, of course.
#endif

//  Simple enumeration item with default value.
#define ENUMINST_VAL1(name) name,
#define ENUMINST_CASERETSTR1(name) case name: return #name;

//  Enumeration item with given value.
#define ENUMINST_VAL2(name, num) name = num,
#define ENUMINST_CASERETSTR2(name, num) case num: return #name;

//  Enumeration item with given value and string representation.
#define ENUMINST_VAL3(name, num, str) name = num,
#define ENUMINST_CASERETSTR3(name, num, str) case num: return str;

//  Enumeration item with different names on C and C++, but same value and string
//  representation on both.

#ifdef __cplusplus
#define ENUMINST_VAL4(cppname, cname, num, str) cppname = num,
#else
#define ENUMINST_VAL4(cppname, cname, num, str) cname = num,
#endif

#define ENUMINST_CASERETSTR4(cppname, cname, num, str) case num: return str;

#define ENUMINST_VAL(...) GET_MACRO4(__VA_ARGS__, ENUMINST_VAL4, ENUMINST_VAL3, ENUMINST_VAL2, ENUMINST_VAL1)(__VA_ARGS__)
#define ENUMINST_CASERETSTR(...) GET_MACRO4(__VA_ARGS__, ENUMINST_CASERETSTR4, ENUMINST_CASERETSTR3, ENUMINST_CASERETSTR2, ENUMINST_CASERETSTR1)(__VA_ARGS__)

#define ENUM_TO_STRING_EX1(enumName, attrBefore, func, val, enumDef) \
    attrBefore char const * const func              \
    {                                               \
        switch ((__underlying_type(enumName))val)   \
        {                                           \
            enumDef(ENUMINST_CASERETSTR)            \
            default:                                \
                return "UNKNOWN";                   \
        }                                           \
    }

#define ENUM_TO_STRING_EX2(enumName, enumDef, cppnamespace) \
    ENUM_TO_STRING_EX1(enumName, , cppnamespace::EnumToString(enumName const val), val, enumDef)

/*#define ENUM_TO_STRING_EX2_DECL(enumName, enumDef, cppnamespace) \
    char const * const cppnamespace::EnumToString(enumName const val)*/

#define ENUM_TO_STRING(enumName, enumDef) \
    ENUM_TO_STRING_EX1(enumName, , EnumToString(enumName const val), val, enumDef)

#define ENUM_TO_STRING_DECL(enumName, enumDef) \
    char const * const EnumToString(enumName const val)

/*  Some type aliases...  */

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

//  I think these names are great... I just like to have 'em here.

/*  Some funnction types...  */

typedef bool (* PredicateFunction0)(void);
typedef void (* ActionFunction0)(void);

/*  Memory barriers and forced ordering  */

#define COMPILER_MEMORY_BARRIER() asm volatile ( "" : : : "memory" )

#define FORCE_ORDER(before, after) asm volatile ( ""                  \
                                                : [out]"=m"(after )   \
                                                : [in ] "m"(before) )

#define with(primExp)                                                   \
    for (bool MCATS(_go_, __LINE__) = true; MCATS(_go_, __LINE__); )    \
    for (primExp; MCATS(_go_, __LINE__); MCATS(_go_, __LINE__) = false)
//  Astonishingly, GCC can optimize this.

#ifdef __BEELZEBUB__DEBUG
    #define onDebug if (true)
    #define onRelease if (false)
#else
    #define onDebug if (false)
    #define onRelease if (true)
#endif

#ifdef __BEELZEBUB_KERNEL
    #define onKernel if (true)
#else
    #define onKernel if (false)
#endif

//  Lock elision helpers

#define ANNOTATE_LOCK_OPERATION(opType) \
asm volatile goto(".pushsection .locks." #opType ", \"a\", @progbits \n\t" \
                  _GAS_DATA_POINTER " %l0 \n\t" \
                  _GAS_DATA_POINTER " %l1 \n\t" \
                  ".popsection \n\t" \
                  : : : : op_start, op_end )

#define ANNOTATE_LOCK_OPERATION_CHK ANNOTATE_LOCK_OPERATION(chk)
#define ANNOTATE_LOCK_OPERATION_ACQ ANNOTATE_LOCK_OPERATION(acq)
#define ANNOTATE_LOCK_OPERATION_REL ANNOTATE_LOCK_OPERATION(rel)

#ifndef EXTEND_POINTER
#define EXTEND_POINTER(ptr) do { } while (false)
#endif

#ifdef __cplusplus
namespace Beelzebub
{
    /*******************
        Scope Guards
    *******************/

    /// <summary>Guards a scope with a specified function.</summary>
    template<typename TFunc>
    struct ScopeGuard
    {
        /*  Constructor(s)  */

        inline ScopeGuard(TFunc const func) : Guard(func) { }

        ScopeGuard(ScopeGuard const &) = delete;
        ScopeGuard(ScopeGuard && other) = default;
        ScopeGuard & operator =(ScopeGuard const &) = delete;
        ScopeGuard & operator =(ScopeGuard &&) = default;

        /*  Destructor  */

        inline ~ScopeGuard()
        {
            this->Guard();
        }

    private:
        /*  Field(s)  */

        TFunc const Guard;
    };

    template<typename TFunc>
    __forceinline ScopeGuard<TFunc> MakeScopeGuard(TFunc const func) { return ScopeGuard<TFunc>(func); }
}
#endif
