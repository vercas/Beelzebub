#pragma once

#include <utils/bitfields.hpp>

#define TAGPTR_BEGIN(name, tagName, tagLen, tagType)                        \
union name                                                                  \
{                                                                           \
    uintptr_t Tagger;                                                       \
    BITFIELD_STRM_RW(0, tagLen, tagType, tagName, uintptr_t, this->Tagger,  \
        __bland, const, static)                                             \
    BITFIELD_STRM_RW(0, tagLen, tagType, Tag    , uintptr_t, this->Tagger,  \
        __bland, const, static)                                             \
    __bland inline constexpr name() : Tagger(0) { }                         \
    __bland inline bool IsNull()                                            \
    {                                                                       \
        return 0 == (this->Tagger & ~TagBits);                              \
    }                                                                       \
    __bland inline bool operator ==(void const * const other)               \
    {                                                                       \
        return (uintptr_t)other == (this->Tagger & ~TagBits);               \
    }                                                                       \
    __bland inline bool operator !=(void const * const other)               \
    {                                                                       \
        return (uintptr_t)other != (this->Tagger & ~TagBits);               \
    }                                                                       \

//  NOTE: The bitfield is declared twice so `tagName` is an alias for "Tag".

#define TAGPTR_TYPE(uName, tagValue, name, type)                            \
    type name;                                                              \
    __bland inline constexpr uName(type val)                                \
        : Tagger((uintptr_t)val | ((uintptr_t)tagValue & TagBits))          \
    {                                                                       \
        /* nothing */                                                       \
    }                                                                       \
    __bland inline uName & operator =(type const val)                       \
    {                                                                       \
        this->Tagger = (uintptr_t)val | ((uintptr_t)tagValue & TagBits);    \
        return *this;                                                       \
    }                                                                       \
    __bland inline type MCATS(Get, name)() const                            \
    {                                                                       \
        if (this->GetTag() == (tagValue))                                   \
            return (type)((uintptr_t)this->name & ~TagBits);                \
        else                                                                \
            return nullptr;                                                 \
    }

#define TAGPTR_END() \
};
