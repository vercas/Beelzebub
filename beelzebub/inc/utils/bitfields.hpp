#pragma once

#include <metaprogramming.h>

#include <utils/bitfields.arc.inc>

/*  Bit properties!!1!  */

#ifndef BITPROPRO
//  Creates a getter for bit-based properties   .
#define BITPROPRO(name, value, decor)                               \
decor inline bool MCATS(Get, name)() const                          \
{                                                                   \
    return 0 != (value & MCATS(name, Bit));                         \
}
#endif

#ifndef BITPROPRW
//  Creates a getter and setter for bit-based properties.
#define BITPROPRW(name, value, decor)                               \
decor inline bool MCATS(Get, name)() const                          \
{                                                                   \
    return 0 != (value & MCATS(name, Bit));                         \
}                                                                   \
decor inline void MCATS(Set, name)(bool const val)                  \
{                                                                   \
    if (val)                                                        \
        value |=  MCATS(name, Bit);                                 \
    else                                                            \
        value &= ~MCATS(name, Bit);                                 \
}                                                                   \
decor inline bool MCATS(FetchSet, name)()                           \
{                                                                   \
    bool res = 0 != (value & MCATS(name, Bit));                     \
                                                                    \
    value |=  MCATS(name, Bit);                                     \
                                                                    \
    return res;                                                     \
}                                                                   \
decor inline bool MCATS(FetchClear, name)()                         \
{                                                                   \
    bool res = 0 != (value & MCATS(name, Bit));                     \
                                                                    \
    value &= ~MCATS(name, Bit);                                     \
                                                                    \
    return res;                                                     \
}                                                                   \
decor inline bool MCATS(FetchFlip, name)()                          \
{                                                                   \
    bool res = 0 != (value & MCATS(name, Bit));                     \
                                                                    \
    value ^=  MCATS(name, Bit);                                     \
                                                                    \
    return res;                                                     \
}
#endif
