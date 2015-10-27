#pragma once

#include <metaprogramming.h>

#include <utils/bitfields.arc.inc>

#define _TEMP_BIT_DEFINE ((varT)1 << bitInd)

/*  Bit properties!!1!  */

#ifndef BITFIELD_FLAG_RO
//  Creates a getter for bit-based flags.
#define BITFIELD_FLAG_RO(bitInd, name, varT, var, decB, decG)                  \
decB inline bool MCATS(Get, name)() decG                                       \
{                                                                              \
    return 0 != (var & _TEMP_BIT_DEFINE);                                      \
}
#endif

#ifndef BITFIELD_FLAG_RW
//  Creates a getter and setter for bit-based flags.
#define BITFIELD_FLAG_RW(bitInd, name, varT, var, decB, decG)                  \
decB inline bool MCATS(Get, name)() decG                                       \
{                                                                              \
    return 0 != (var & _TEMP_BIT_DEFINE);                                      \
}                                                                              \
decB inline void MCATS(Set, name)(bool const val)                              \
{                                                                              \
    if (val)                                                                   \
        var |=  _TEMP_BIT_DEFINE;                                              \
    else                                                                       \
        var &= ~_TEMP_BIT_DEFINE;                                              \
}                                                                              \
decB inline bool MCATS(FetchSet, name)()                                       \
{                                                                              \
    bool res = 0 != (var & _TEMP_BIT_DEFINE);                                  \
                                                                               \
    var |=  _TEMP_BIT_DEFINE;                                                  \
                                                                               \
    return res;                                                                \
}                                                                              \
decB inline bool MCATS(FetchClear, name)()                                     \
{                                                                              \
    bool res = 0 != (var & _TEMP_BIT_DEFINE);                                  \
                                                                               \
    var &= ~_TEMP_BIT_DEFINE;                                                  \
                                                                               \
    return res;                                                                \
}                                                                              \
decB inline bool MCATS(FetchFlip, name)()                                      \
{                                                                              \
    bool res = 0 != (var & _TEMP_BIT_DEFINE);                                  \
                                                                               \
    var ^=  _TEMP_BIT_DEFINE;                                                  \
                                                                               \
    return res;                                                                \
}
#endif

#ifndef BITFIELD_STRM_RO
//  Creates a getter for bit-based masked strings.
#define BITFIELD_STRM_RO(bitInd, bitLen, fldT, name, varT, var, decB, decG)    \
decB inline fldT MCATS(Get, name)() decG                                       \
{                                                                              \
    return (fldT)(var & MCATS(name, Bits));                                    \
}
#endif

#ifndef BITFIELD_STRM_RW
//  Creates a getter and setter for bit-based masked strings.
#define BITFIELD_STRM_RW(bitInd, bitLen, fldT, name, varT, var, decB, decG)    \
decB inline fldT MCATS(Get, name)() decG                                       \
{                                                                              \
    return (fldT)(var & MCATS(name, Bits));                                    \
}                                                                              \
decB inline void MCATS(Set, name)(fldT const val)                              \
{                                                                              \
    var = ((varT)val &  MCATS(name, Bits))                                     \
        | (      var & ~MCATS(name, Bits));                                    \
}
#endif

#ifndef BITFIELD_STRO_RO
//  Creates a getter for bit-based masked & offset strings.
#define BITFIELD_STRO_RO(bitInd, bitLen, fldT, name, varT, var, decB, decG)    \
decB inline fldT MCATS(Get, name)() decG                                       \
{                                                                              \
    return (fldT)((var & MCATS(name, Bits)) >> MCATS(name, Offset));           \
}
#endif

#ifndef BITFIELD_STRO_RW
//  Creates a getter and setter for bit-based masked & offset strings.
#define BITFIELD_STRO_RW(bitInd, bitLen, fldT, name, varT, var, decB, decG)    \
decB inline fldT MCATS(Get, name)() decG                                       \
{                                                                              \
    return (fldT)((var & MCATS(name, Bits)) >> MCATS(name, Offset));           \
}                                                                              \
decB inline void MCATS(Set, name)(fldT const val)                              \
{                                                                              \
    var = (((varT)val << MCATS(name, Offset)) &  MCATS(name, Bits))            \
        | (       var                         & ~MCATS(name, Bits));           \
}
#endif
