/**
 *  The loops here should be perfectly compatible with single machine code
 *  instructions. Maybe with a branch or two mixed in.
 */

#include <string.h>

const void * memchr(const void * const src, const int val, size_t len)
{
    const byte * s = (byte *)src;

    for (; len > 0; ++s, --len)
        if (*s == val)
            return s;

    return nullptr;
}

void * memchr(void * const src, const int val, size_t len)
{
    const byte * s = (byte *)src;

    for (; len > 0; ++s, --len)
        if (*s == val)
            return s;

    return nullptr;
}
