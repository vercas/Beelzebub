#pragma once

#include <metaprogramming.h>

#include <utils/bitfields.arc.inc>

#define _TEMP_BIT_DEFINE ((varT)1 << bitInd)

/*  Bit properties!!1!  */

#ifndef BITFIELD_FLAG_RO
//  Creates a getter for bit-based properties   .
#define BITFIELD_FLAG_RO(bitInd, name, varT, var, decB, decG)                  \
decB inline bool MCATS(Get, name)() decG                                       \
{                                                                              \
    return 0 != (var & _TEMP_BIT_DEFINE);                                      \
}
#endif

#ifndef BITFIELD_FLAG_RW
//  Creates a getter and setter for bit-based properties.
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
