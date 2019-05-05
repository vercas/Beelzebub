/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#pragma once

#if defined(__ASSEMBLER__)
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#endif

#if defined(__cplusplus)
    #define __BEELZEBUB__SOURCE_CXX
    #define __BEELZEBUB__SOURCE CXX
#elif defined(__ASSEMBLER__)
    #define __BEELZEBUB__SOURCE_GAS
    #define __BEELZEBUB__SOURCE GAS
#else
    #define __BEELZEBUB__SOURCE_C
    #define __BEELZEBUB__SOURCE C
#endif

#ifndef __BEELZEBUB__SOURCE_GAS
#include <stdint.h>
#include <stddef.h>
#endif

#if defined(__BEELZEBUB_KERNEL) || defined(__BEELZEBUB_KERNEL_MODULE)
    #define __BEELZEBUB__IN_KERNEL
#endif

#ifdef __BEELZEBUB__SOURCE_C
    #include <stdbool.h>
#endif

/***********************
    Macro Assistance
***********************/

#define GET_ARG1(_1) _1
#define GET_ARG2(_2, _1) _1
#define GET_ARG3(_3, _2, _1) _1
#define GET_ARG4(_4, _3, _2, _1) _1

#ifndef __BEELZEBUB__SOURCE_GAS
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
#endif

#define MCATS1(A) A
#define MCATS2(A, B) A##B
#define MCATS3(A, B, C) A##B##C
#define MCATS4(A, B, C, D) A##B##C##D
#define MCATS5(A, B, C, D, E) A##B##C##D##E
#define MCATS6(A, B, C, D, E, F) A##B##C##D##E##F

#ifndef __BEELZEBUB__SOURCE_GAS
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
#endif

#define LINEVAR(name) MCATS(name, _, __LINE__)

#ifdef __COUNTER__
    #define ANONVAR(name) MCATS(name, _, __COUNTER__)
#else
    #define ANONVAR(name) LINEVAR(name)
#endif

/****************
    Constants
****************/

#ifdef __BEELZEBUB__SOURCE_CXX
    #define nullpaddr (paddr_t{ 0 })
    #define nullvaddr (vaddr_t{ nullptr })
#else
    #define nullptr   ((void *) 0)
    #define nullpaddr ((paddr_t)0)
    #define nullvaddr ((vaddr_t)0)
#endif

/*****************
    Attributes
*****************/

#ifndef __BEELZEBUB__SOURCE_GAS
    #ifdef __BEELZEBUB__SOURCE_CXX
        #define __extern extern "C"
    #else
        #define __extern extern
    #endif

    /*  This part defines a few function modifiers based on attributes. */

    #ifdef __GNUC__
        #define __forceinline      inline  __attribute__((__always_inline__))
        #define __noinline         __attribute__((__noinline__))
        #define __noclone          __attribute__((__noclone__))
        #define __solid            __attribute__((__noinline__, __noclone__))
        #define __artificial       __forceinline __attribute__((__artificial__))

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
        #define __symbol(sym)      __asm__(#sym)
        #define __must_check       __attribute__((__warn_unused_result__))
        #define __restrict         __restrict__
        #define __nonnull(...)     __attribute__((__nonnull__(__VA_ARGS__)))
        #define __returns_nonnull  __attribute__((__returns_nonnull__))
        #define __malloc           __attribute__((__malloc__))
        #define __realign_stack    __attribute__((__force_align_arg_pointer__))

        #define likely(expr)       (__builtin_expect(!!(expr), 1))
        #define unlikely(expr)     (__builtin_expect(!!(expr), 0))
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

        #define __shared            __extern __attribute__((__used__, __visibility__("default"), __externally_visible__, __noinline__, __noclone__))
        #define __shared_inline     __extern __attribute__((__used__, __visibility__("default"), __externally_visible__))
        #define __shared_cpp        __attribute__((__used__, __visibility__("default"), __externally_visible__, __noinline__, __noclone__))
        #define __shared_cpp_inline __attribute__((__used__, __visibility__("default"), __externally_visible__))

        #define __public           __attribute__((__visibility__("default")))
        #define __protected        __attribute__((__visibility__("protected")))
        #define __internal         __attribute__((__visibility__("internal")))

        #define __deprecated(txt)  __attribute__((__deprecated__(txt)))
    #else
        #error Unsupported compiler!

        #define __forceinline      inline
        #define __noinline  
        #define __noclone  
        #define __solid  

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
        #define __realign_stack  

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
#endif

/******************
    C++ Support
******************/

#if defined(__BEELZEBUB__SOURCE_CXX) && !defined(__BEELZEBUB__PLACEMENT_NEW)
    #define __BEELZEBUB__PLACEMENT_NEW
    
    __forceinline void * operator new     (size_t, void * p) { return p; }
    __forceinline void * operator new[]   (size_t, void * p) { return p; }
    __forceinline void   operator delete  (void *, void *  ) { }
    __forceinline void   operator delete[](void *, void *  ) { }
#endif

/************************
    Naming Assistance
************************/

#ifdef __BEELZEBUB__SOURCE_CXX
    #define __NAMESPACE_BEGIN        namespace Beelzebub {
    #define __NAMESPACE_END          }

    #define __EXTERN_C_BEGIN         extern "C" {
    #define __EXTERN_C_END           }

    #define __NAMESPACED(ns, name)   Beelzebub::name
#else
    #define __NAMESPACE_BEGIN  
    #define __NAMESPACE_END  

    #define __EXTERN_C_BEGIN  
    #define __EXTERN_C_END  

    #define __NAMESPACED(ns, name)   ns ## name
#endif

//#define __BE(name) __NAMESPACED(Be, name)

/****************************
    Enumarator Assistance
****************************/

#ifdef __BEELZEBUB__SOURCE_CXX
    #define __ENUMOPS_LITE2(T, U)                                                           \
        __artificial bool operator == (U   a, T b) { return               a  == (U )(b);  } \
        __artificial bool operator != (U   a, T b) { return               a  != (U )(b);  } \
        __artificial bool operator == (T   a, U b) { return         (U  )(a) ==      b ;  } \
        __artificial bool operator != (T   a, U b) { return         (U  )(a) !=      b ;  }

    #define __ENUMOPS_FULL2(T, U)                                                           \
        __artificial  T   operator ~  (T   a     ) { return (T  )(~((U  )(a))          ); } \
        __artificial  T   operator |  (T   a, T b) { return (T  )(  (U  )(a) |  (U )(b)); } \
        __artificial  T   operator &  (T   a, T b) { return (T  )(  (U  )(a) &  (U )(b)); } \
        __artificial  T   operator &  (T   a, U b) { return (T  )(  (U  )(a) &       b ); } \
        __artificial  T   operator ^  (T   a, T b) { return (T  )(  (U  )(a) ^  (U )(b)); } \
        __artificial  T & operator |= (T & a, T b) { return (T &)(  (U &)(a) |= (U )(b)); } \
        __artificial  T & operator &= (T & a, T b) { return (T &)(  (U &)(a) &= (U )(b)); } \
        __artificial  T & operator ^= (T & a, T b) { return (T &)(  (U &)(a) ^= (U )(b)); } \
        __ENUMOPS_LITE2(T, U)

    #define __ENUMOPS_LITE1(T) __ENUMOPS_LITE2(T, __underlying_type(T))
    #define __ENUMOPS_FULL1(T) __ENUMOPS_FULL2(T, __underlying_type(T))
    //  All nice and dandy, but it uses a GCC extension for type traits because
    //  the type_traits.h header is unavailable.

    #define __ENUMOPS_LITE(...) GET_MACRO2(__VA_ARGS__, __ENUMOPS_LITE2, __ENUMOPS_LITE1)(__VA_ARGS__)
    #define __ENUMOPS_FULL(...) GET_MACRO2(__VA_ARGS__, __ENUMOPS_FULL2, __ENUMOPS_FULL1)(__VA_ARGS__)

    #define __ENUMOPS(...) __ENUMOPS_FULL(__VA_ARGS__)

    //  Why? For the glory of C++, of course.

    //  Simple enumerator item with default value.
    #define __ENUMINST_VAL1(name) name,
    //  Enumerator item with given value.
    #define __ENUMINST_VAL2(name, num) name = num,
    //  Enumerator item with given value and string representation.
    #define __ENUMINST_VAL3(name, num, str) name = num,
    //  Enumerator item with different names on C and C++, but same value and string representation on both.
    #define __ENUMINST_VAL4(cppname, cname, num, str) cppname = num,

    //  Enumerator item with all four possibilities above.
    #define __ENUMINST_VAL(...) GET_MACRO4(__VA_ARGS__, __ENUMINST_VAL4, __ENUMINST_VAL3, __ENUMINST_VAL2, __ENUMINST_VAL1)(__VA_ARGS__)

    //  Integer-based enumerator.
    #define __ENUMDECL2(name, macro) \
        enum class name { macro(__ENUMINST_VAL) };

    //  Integer enumerator with operators.
    #define __ENUMDECL3(name, macro, type)          \
        enum class name { macro(__ENUMINST_VAL) };  \
        MCATS(__ENUMOPS_, type)(name)

    //  Typed enumerator with operators.
    #define __ENUMDECL4(name, macro, type, inner)           \
        enum class name : inner { macro(__ENUMINST_VAL) };  \
        MCATS(__ENUMOPS_, type)(name, inner)

    //  Abstraction over the three possibilities above.
    #define __ENUMDECL(name, ...) \
        GET_MACRO3(__VA_ARGS__, __ENUMDECL4, __ENUMDECL3, __ENUMDECL2)(name, __VA_ARGS__)

    //  Public API enumerator declaration, based on same possibilities.
    #define __PUB_ENUM(name, ...) \
        namespace Beelzebub { __ENUMDECL(name, __VA_ARGS__) } \
        using MCATS(Be, name) = Beelzebub::name;

    //  Simple enumerator item with default value.
    #define __ENUMINST_CASERETSTR1(name) case name: return #name;
    //  Enumerator item with given value.
    #define __ENUMINST_CASERETSTR2(name, num) case num: return #name;
    //  Enumerator item with given value and string representation.
    #define __ENUMINST_CASERETSTR3(name, num, str) case num: return str;
    //  Enumerator item with different names on C and C++, but same value and string representation on both.
    #define __ENUMINST_CASERETSTR4(cppname, cname, num, str) case num: return str;

    //  Abstraction over the 4 cases above.
    #define __ENUMINST_CASERETSTR(...) GET_MACRO4(__VA_ARGS__, __ENUMINST_CASERETSTR4, __ENUMINST_CASERETSTR3, __ENUMINST_CASERETSTR2, __ENUMINST_CASERETSTR1)(__VA_ARGS__)

    /*#define __ENUM_TO_STRING_EX1(enumName, attrBefore, func, val, enumDef) \
        attrBefore char const * func                    \
        {                                               \
            switch ((__underlying_type(enumName))val)   \
            {                                           \
                enumDef(__ENUMINST_CASERETSTR)          \
                default:                                \
                    return "UNKNOWN";                   \
            }                                           \
        }

    #define __ENUM_TO_STRING_EX2(enumName, enumDef, cppnamespace) \
        __ENUM_TO_STRING_EX1(enumName, , cppnamespace::EnumToString(enumName const val), val, enumDef)

    #define __ENUM_TO_STRING(enumName, enumDef) \
        __ENUM_TO_STRING_EX1(enumName, , EnumToString(enumName const val), val, enumDef)

    #define __ENUM_TO_STRING_IMPL(enumName, enumDef) \
        __ENUM_TO_STRING_EX1(enumName, , EnumToString(enumName const val), val, enumDef)*/

    #define __ENUM_TO_STRING_IMPL2(enumName, enumDef)   \
    namespace Beelzebub {                               \
        char const * EnumToString(enumName const val)   \
        {                                               \
            switch ((__underlying_type(enumName))val)   \
            {                                           \
                enumDef(__ENUMINST_CASERETSTR)          \
            default:                                    \
                return "UNKNOWN";                       \
            }                                           \
        }                                               \
        namespace Terminals                             \
        {                                               \
            template<>                                  \
            TerminalBase & operator << <enumName>(TerminalBase & term, enumName const val)  \
            {                                           \
                return term << (__underlying_type(enumName))(val) << " (" << EnumToString(val) << ")";  \
            }                                           \
            template<>                                  \
            TerminalWriteResult WriteFormattedValue<enumName>(TerminalBase * term, enumName const * val, char const * format)   \
            {                                           \
                return WriteFormattedEnumHelper(term, (uint64_t)(*val), format, [](uint64_t val64) { return EnumToString((enumName)val64); }, sizeof(enumName));    \
            }                                           \
        }                                               \
    }

    #define __ENUM_TO_STRING_IMPL3(enumName, enumDef, cppNamespace) \
    namespace cppNamespace {                            \
        char const * EnumToString(enumName const val)   \
        {                                               \
            switch ((__underlying_type(enumName))val)   \
            {                                           \
                enumDef(__ENUMINST_CASERETSTR)          \
            default:                                    \
                return "UNKNOWN";                       \
            }                                           \
        }                                               \
    }                                                   \
    namespace Beelzebub { namespace Terminals           \
    {                                                   \
        template<>                                      \
        TerminalBase & operator << <enumName>(TerminalBase & term, enumName const val)  \
        {                                               \
            return term << (__underlying_type(enumName))(val) << " (" << cppNamespace::EnumToString(val) << ")";    \
        }                                               \
        template<>                                      \
        TerminalWriteResult WriteFormattedValue<enumName>(TerminalBase * term, enumName const * val, char const * format)   \
        {                                               \
            return WriteFormattedEnumHelper(term, (uint64_t)(*val), format, [](uint64_t val64) { return cppNamespace::EnumToString((enumName)val64); }, sizeof(enumName));    \
        }                                               \
    }}

    #define __ENUM_TO_STRING_IMPL(enumName, ...) GET_MACRO2(__VA_ARGS__, __ENUM_TO_STRING_IMPL3, __ENUM_TO_STRING_IMPL2)(enumName, __VA_ARGS__)

    #define __ENUM_TO_STRING_DECL(enumName) \
        char const * EnumToString(enumName const val);
#elif !defined(__ASSEMBLER__)
    //  Simple enumerator item with default value.
    #define __ENUMINST_VAL1(name) 
    //  Enumerator item with given value.
    #define __ENUMINST_VAL2(name, num) static const name = num;
    //  Enumerator item with given value and string representation.
    #define __ENUMINST_VAL3(name, num, str) name = num,
    //  Enumerator item with different names on C and C++, but same value and string representation on both.
    #define __ENUMINST_VAL4(cppname, cname, num, str) cppname = num,

    //  Enumerator item with all four possibilities above.
    #define __ENUMINST_VAL(...) GET_MACRO4(__VA_ARGS__, __ENUMINST_VAL4, __ENUMINST_VAL3, __ENUMINST_VAL2, __ENUMINST_VAL1)(__VA_ARGS__)

    //  Integer-based enumerator.
    #define __ENUMDECL2(name, macro) \
        typedef int Be##name;

    //  Integer enumerator with operators.
    #define __ENUMDECL3(name, macro, type)          \
        typedef int Be##name;

    //  Typed enumerator with operators.
    #define __ENUMDECL4(name, macro, type, inner)           \
        typedef inner Be##name;

    //  Abstraction over the three possibilities above.
    #define __ENUMDECL(name, ...) \
        GET_MACRO3(__VA_ARGS__, __ENUMDECL4, __ENUMDECL3, __ENUMDECL2)(name, __VA_ARGS__)

    //  Public API enumerator declaration, based on same possibilities.
    #define __PUB_ENUM(name, ...) \
        __ENUMDECL(name, __VA_ARGS__)
#endif

#ifdef __BEELZEBUB__SOURCE_CXX
    #ifdef __BEELZEBUB__SOURCE_CXX
    #else
        #define __ENUMINST_VAL4(cppname, cname, num, str) cname = num,
    #endif


#endif

/**************************
    Function Assistance
**************************/

#if defined(__BEELZEBUB__SOURCE_CXX)
    #define __PUB_FUNC(ret, name, ...) \
        __shared __deprecated("The Be-prefixed version should be avoided in C++") ret Be##name(__VA_ARGS__) __symbol(__deprecated_Be##name); \
        namespace Beelzebub { __shared_cpp ret name(__VA_ARGS__) __symbol(Be##name); }
#elif defined(__BEELZEBUB__SOURCE_C)
    #define __PUB_FUNC(ret, name, ...) \
        __shared ret MCATS(Be, name)(__VA_ARGS__);
#endif

/***************************
    Structure Assistance
***************************/

#if defined(__BEELZEBUB__SOURCE_CXX)
    #define __FORWARD(name) \
    struct name; \
    using MCATS(Be, name) = name;

    #define __STRUCT(name) \
    __FORWARD(name) \
    struct name
#elif defined(__BEELZEBUB__SOURCE_C)
    #define __FORWARD(name) \
    struct MCATS(Be, name, _s); \
    typedef MCATS(Be, name, _s) MCATS(Be, name);

    #define __STRUCT(name) \
    __FORWARD(name) \
    struct MCATS(Be, name, _s)
#else
    //  Field reference (name)
    #define FIELDR(n, f)    MCATS3(n, _, f)
    //  Field w/ size (declaration)
    #define FIELDS(n, f, s) FIELDR(n, f): .struct FIELDR(n, f) + s
    //  Field w/ type (declaration; size deduced from type)
    #define FIELDT(n, f, t) FIELDS(n, f, sizeof(t))

    #define sizeof(type)    MCATS2(SIZE_OF_, type)

    #define SIZE_OF_int8_t    1
    #define SIZE_OF_int16_t   2
    #define SIZE_OF_int32_t   4
    #define SIZE_OF_int64_t   8
    #define SIZE_OF_int128_t  16
    #define SIZE_OF_uint8_t   1
    #define SIZE_OF_uint16_t  2
    #define SIZE_OF_uint32_t  4
    #define SIZE_OF_uint64_t  8
    #define SIZE_OF_uint128_t 16
#endif

/*****************
    Some Types
*****************/

#ifndef __BEELZEBUB__SOURCE_GAS
    __NAMESPACE_BEGIN

    typedef union paddr_u
    {
        paddr_inner_t Value;

        #ifdef __BEELZEBUB__SOURCE_CXX
            __artificial explicit constexpr paddr_u() : Value() { }
            __artificial explicit constexpr paddr_u(paddr_inner_t val) : Value(val) { }

            __artificial explicit operator paddr_inner_t() const { return this->Value; }
        #endif
    } paddr_t;

    typedef union psize_u
    {
        psize_inner_t Value;

        #ifdef __BEELZEBUB__SOURCE_CXX
            __artificial explicit constexpr psize_u() : Value() { }
            __artificial explicit constexpr psize_u(psize_inner_t val) : Value(val) { }

            __artificial explicit operator psize_inner_t() const { return this->Value; }
        #endif
    } psize_t;

    typedef union vaddr_u
    {
        vaddr_inner_t Value;
        void const * Pointer;

        #ifdef __BEELZEBUB__SOURCE_CXX
            __artificial explicit constexpr vaddr_u() : Value() { }
            __artificial explicit constexpr vaddr_u(vaddr_inner_t val) : Value(val) { }
            __artificial explicit constexpr vaddr_u(void const * ptr) : Pointer(ptr) { }

            __artificial explicit operator vaddr_inner_t() const { return this->Value; }
            __artificial explicit operator void const *() const { return this->Pointer; }
            __artificial explicit operator void *() const { return const_cast<void *>(this->Pointer); }
        #endif
    } vaddr_t;

    typedef union vsize_u
    {
        vsize_inner_t Value;

        #ifdef __BEELZEBUB__SOURCE_CXX
            __artificial explicit constexpr vsize_u() : Value() { }
            __artificial explicit constexpr vsize_u(vsize_inner_t val) : Value(val) { }
            __artificial explicit constexpr vsize_u(psize_t const val) : Value(val.Value) { }

            __artificial explicit operator vsize_inner_t() const { return this->Value; }

            __artificial operator psize_t() const { return psize_t { this->Value }; }
        #endif
    } vsize_t;

    #ifdef __BEELZEBUB__SOURCE_CXX
        typedef union PageSize_u
        {
            page_size_inner_t Value;

            __artificial explicit constexpr PageSize_u(page_size_inner_t const val) : Value(val) { }

            __artificial operator psize_t() const { return psize_t { this->Value }; }
            __artificial operator vsize_t() const { return vsize_t { this->Value }; }

            __artificial operator page_size_inner_t() const { return this->Value; }
        } PageSize_t;

        #define CMP_OP(TYP, OP) \
            __artificial constexpr bool operator OP (TYP a, TYP b) { return a.Value OP b.Value; }
        #define CMP_OP2(Ta, Tb, OP) \
            __artificial constexpr bool operator OP (Ta a, Tb b) { return a.Value OP b; } \
            __artificial constexpr bool operator OP (Tb a, Ta b) { return a OP b.Value; }
        #define CMP_OP3(Ta, Tb, OP) \
            __artificial constexpr bool operator OP (Ta a, Tb b) { return a.Value OP b.Value; } \
            __artificial constexpr bool operator OP (Tb a, Ta b) { return a.Value OP b.Value; }
        #define ARI_OP(Tr, Ta, Tb, OP) \
            __artificial constexpr Tr operator OP (Ta const a, Tb const b) { return Tr { a.Value OP b.Value }; }
        #define EQU_OP(Tt, To, OP) \
            __artificial constexpr Tt & operator MCATS(OP, =) (Tt & t, To const o) { t.Value MCATS(OP, =) o.Value; return t; }

        #define CMP_OPS(TYP, INN) \
            CMP_OP(TYP, ==) CMP_OP2(TYP, INN, ==) CMP_OP(TYP, !=) CMP_OP2(TYP, INN, !=) \
            CMP_OP(TYP,  <) CMP_OP2(TYP, INN,  <) CMP_OP(TYP, <=) CMP_OP2(TYP, INN, <=) \
            CMP_OP(TYP,  >) CMP_OP2(TYP, INN,  >) CMP_OP(TYP, >=) CMP_OP2(TYP, INN, >=)

        #define CMP_OPS2(Ta, Tb) \
            CMP_OP3(Ta, Tb, ==) CMP_OP3(Ta, Tb, !=) \
            CMP_OP3(Ta, Tb,  <) CMP_OP3(Ta, Tb, <=) \
            CMP_OP3(Ta, Tb,  >) CMP_OP3(Ta, Tb, >=)

        CMP_OPS(paddr_t, paddr_inner_t) CMP_OPS(psize_t, psize_inner_t)
        CMP_OPS(vaddr_t, vaddr_inner_t) CMP_OPS(vsize_t, vsize_inner_t)
        CMP_OPS2(psize_t, PageSize_t)
        CMP_OPS2(vsize_t, PageSize_t)
        #undef CMP_OPS
        #undef CMP_OPS2

        #define ARI_OPS(Ta, Ts) \
            ARI_OP(Ta, Ta, Ts, +) ARI_OP(Ta, Ts, Ta, +) \
            ARI_OP(Ta, Ta, Ts, -) ARI_OP(Ts, Ta, Ta, -) \
            ARI_OP(Ts, Ts, Ts, +) ARI_OP(Ts, Ts, Ts, -) \
            ARI_OP(Ts, Ta, Ts, %) ARI_OP(Ts, Ts, Ts, %) \
            EQU_OP(Ta, Ts, +) EQU_OP(Ta, Ts, -) EQU_OP(Ts, Ts, +) EQU_OP(Ts, Ts, -)

        ARI_OPS(paddr_t, psize_t) ARI_OPS(vaddr_t, vsize_t)
        #undef ARI_OPS

        #define ARI_PS_OPS(Ta, Ts) \
            ARI_OP(Ta, Ta, PageSize_t, +) ARI_OP(Ta, Ta, PageSize_t, -) \
            ARI_OP(Ts, Ts, PageSize_t, +) ARI_OP(Ts, Ts, PageSize_t, -) \
            ARI_OP(Ts, Ta, PageSize_t, %) ARI_OP(Ts, Ts, PageSize_t, %) \
            EQU_OP(Ta, PageSize_t, +) EQU_OP(Ta, PageSize_t, -) \
            EQU_OP(Ts, PageSize_t, +) EQU_OP(Ts, PageSize_t, -)

        ARI_PS_OPS(paddr_t, psize_t) ARI_PS_OPS(vaddr_t, vsize_t)
        #undef ARI_PS_OPS

        #undef CMP_OP
        #undef CMP_OP2
        #undef CMP_OP3
        #undef ARI_OP
        #undef EQU_OP

        #define ARI_OP1(Tr, Ta, Tb, OP) \
            __artificial constexpr Tr operator OP (Ta const a, Tb const b) { return Tr { a.Value OP b }; }
        #define ARI_OP2(Tr, Ta, Tb, OP) \
            __artificial constexpr Tr operator OP (Ta const a, Tb const b) { return Tr { a OP b.Value }; }

        #define ARI_SCALE_OPS(Ts) \
            ARI_OP1(Ts, Ts, size_t, *) ARI_OP2(Ts, size_t, Ts, *) \
            ARI_OP1(Ts, Ts, size_t, /) ARI_OP2(Ts, size_t, Ts, /) \
            ARI_OP1(Ts, Ts, size_t, %) ARI_OP2(Ts, size_t, Ts, %) \
            ARI_OP1(Ts, Ts, int, *) ARI_OP2(Ts, int, Ts, *) \
            ARI_OP1(Ts, Ts, int, /) ARI_OP2(Ts, int, Ts, /) \
            ARI_OP1(Ts, Ts, int, %) ARI_OP2(Ts, int, Ts, %)

        ARI_SCALE_OPS(PageSize_t) ARI_SCALE_OPS(psize_t) ARI_SCALE_OPS(vsize_t)
        #undef ARI_SCALE_OPS

        #undef ARI_OP1
        #undef ARI_OP2

        template<typename T>
        static constexpr vsize_t const SizeOf { sizeof(T) };

        template<typename T>
        static constexpr vsize_t const AlignOf { __alignof(T) };
    #endif  //  C++

    /*  Some funnction types...  */

    typedef bool (* PredicateFunction0)(void);
    typedef void (* ActionFunction0)(void);

    __NAMESPACE_END //  namespace Beelzebub
#endif  //  Non-GAS

/*******************************
    Miscellaneous Assistance
*******************************/

#ifdef __BEELZEBUB__SOURCE_C
    #define REINTERPRET_CAST(type, val) ((type)(val))
#elif defined(__BEELZEBUB__SOURCE_CXX)
    #define REINTERPRET_CAST(type, val) (reinterpret_cast<type>(val))
#endif

#ifndef __BEELZEBUB__SOURCE_GAS
    #define COMPILER_MEMORY_BARRIER() asm volatile ( "" : : : "memory" )

    #define FORCE_ORDER(before, after) asm volatile ( ""                  \
                                                    : [out]"=m"(after )   \
                                                    : [in ] "m"(before) )

    #define FORCE_EVAL(before) asm volatile ( "" : : "rm"(before) )

    #define EMPTY_STATEMENT asm volatile ( "" )

    #define with(primExp)                                                   \
        for (bool MCATS(_go_, __LINE__) = true; MCATS(_go_, __LINE__); )    \
        for (primExp; MCATS(_go_, __LINE__); MCATS(_go_, __LINE__) = false)
    //  Astonishingly, GCC can optimize this.

    #ifdef __BEELZEBUB__CONF_DEBUG
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

    #define EXTEND_POINTER(ptr) __extension__ ({ ptr; })
    #define GET_EXTENDED_POINTER(ptr) __extension__ ({ ptr; })

    #define PUT_IN_REGISTER(reg, val) register __typeof__((val)) reg asm(#reg) = (val)
    #define REGISTER_VARIABLE(reg) register uintptr_t reg asm(#reg)
#endif

#ifdef __BEELZEBUB__SOURCE_CXX
    #define PTR_ADD(P, V) (reinterpret_cast<decltype(P)>(reinterpret_cast<uintptr_t>(P) + (V)))
    #define PTR_INC(P, V) ((P) = reinterpret_cast<decltype(P)>(reinterpret_cast<uintptr_t>(P) + (V)))
#else
    #define PTR_ADD(P, V) ((__typeof__(P))((uintptr_t)(P) + (V)))
#endif

/******************************
    Lock Elision Assistance
******************************/

#ifndef __BEELZEBUB__SOURCE_GAS
    #define ANNOTATE_LOCK_OPERATION(opType) \
    asm volatile goto(".pushsection .locks." #opType ", \"a\", @progbits \n\t" \
                      _GAS_DATA_POINTER " %l0 \n\t" \
                      _GAS_DATA_POINTER " %l1 \n\t" \
                      ".popsection \n\t" \
                      : : : : op_start, op_end )

    #define ANNOTATE_LOCK_OPERATION_CHK ANNOTATE_LOCK_OPERATION(chk)
    #define ANNOTATE_LOCK_OPERATION_ACQ ANNOTATE_LOCK_OPERATION(acq)
    #define ANNOTATE_LOCK_OPERATION_REL ANNOTATE_LOCK_OPERATION(rel)
#endif

/******************
    More Stuffs
******************/

#ifndef __BEELZEBUB__SOURCE_GAS
    #define ASSUME(cond) __extension__ ({ if (!(cond)) { __unreachable_code; } })
#endif
