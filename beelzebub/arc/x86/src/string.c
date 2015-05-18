/**
 *  The loops here should be perfectly compatible with single machine code
 *  instructions. Maybe with a branch or two mixed in.
 */

#include <string.h>

bool memeq(const void * const src1, const void * const src2, size_t len)
{
    if (src1 == src2)
        return true;

    const byte * s1 = (byte *)src1;
    const byte * s2 = (byte *)src2;

    for (; len > 0; ++s1, ++s2, --len)
        if (*s1 != *s2)
            return false;

    return true;
}

comp_t memcmp(const void * const src1, const void * const src2, size_t len)
{
    if (src1 == src2)
        return 0;

    const byte * s1 = (byte *)src1;
    const byte * s2 = (byte *)src2;

    sbyte res = 0;    //  Used to store subtraction/comparison results.

    for (; len > 0; ++s1, ++s2, --len)
        if ((res = (sbyte)(*s1) - (sbyte)(*s2)) != 0)
            return (comp_t)res;

    return 0;
}

void * memchr(const void * const src, const int val, size_t len)
{
    const byte * s = (byte *)src;

    for (; len > 0; ++s, --len)
        if (*s == val)
            return (void *)s;   //  Need to discard the `const` qualifier.

    return nullptr;
}

void * memcpy(void * const dst, const void * const src, size_t len)
{
    if (src == dst)
        return dst;

          byte * d = (byte *)dst;
    const byte * s = (byte *)src;

    for (; len > 0; ++d, ++s, --len)
        *d = *s;

    return dst;
}

void * memmove(void * const dst, const void * const src, size_t len)
{
    if (src == dst)
        return dst;

          byte * d = (byte *)dst;
    const byte * s = (byte *)src;

    if (dst < src)
        for (; len > 0; ++d, ++s, --len)
            *d = *s;
    else
    {
        size_t i = 0;

        for (; i < len; ++d, ++s, ++i)
            *d = *s;
    }

    return dst;
}

void * memset(void * const dst, const int val, size_t len)
{
          byte * d = (byte *)dst;
    const byte   v = (byte  )val;

    for (; len > 0; ++d, --len)
        *d = v;

    return dst;
}

