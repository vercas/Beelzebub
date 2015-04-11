/**
 * Copyright (c) 2012 by Lukas Heidemann <lukasheidemann@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <stdint.h>

uintptr_t memalign(uintptr_t address, size_t boundary)
{
    size_t div = address / boundary;
    size_t mod = address - div * boundary;

    if (0 != mod)
        return address + boundary - mod;
    else
        return address;
}

bool memcmp(void *a, void *b, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i)
        if (((uint8_t *) a)[i] != ((uint8_t *) b)[i]) return 0;

    return 1;
}

void memcpy(void *dest, void *src, size_t length)
{
    size_t i;

    for (i = 0; i < length; ++i)
        ((uint8_t *) dest)[i] = ((uint8_t *) src)[i];
}

void memset(void *dest, uint8_t c, size_t length)
{
    size_t i;

    for (i = 0; i < length; ++i)
        ((uint8_t *) dest)[i] = c;
}

uint8_t strcmp(const int8_t *a, const int8_t *b)
{
    size_t i;
    for (i = 0; 0 != a[i] || 0 != b[i]; ++i)
        if (a[i] != b[i]) return 0;

    return 1;
}

int8_t *strcpy(int8_t *dest, const int8_t *src)
{
    size_t i;
    for (i = 0; 0 != src[i]; ++i)
        dest[i] = src[i];

    dest[i] = 0;

    return dest;
}

size_t strlen(const int8_t *str)
{
    size_t len;
    for (len = 0; 0 != str[len]; ++len);
    return len;
}

int8_t *strstr(const int8_t *haystack, const int8_t *needle)
{
    size_t str_pos, substr_pos;
    substr_pos = 0;

    if (0 == needle[0])
        return (int8_t *) haystack;

    for (str_pos = 0; 0 != haystack[str_pos]; ++str_pos) {
        if (needle[substr_pos] == haystack[str_pos]) {
            ++substr_pos;

            if (0 == needle[substr_pos])
                return (int8_t *)
                (((uintptr_t) haystack) + str_pos - substr_pos + 1);
        } else {
            substr_pos = 0;
        }
    }

    return (int8_t *) 0;
}

