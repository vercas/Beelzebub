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

/** 
 *  Yeah, there are C++-specific declarations too.
 *  The C and common ones are defined in 'string.c'.
 *  The C++-specific ones are defined in 'string.cpp'.
 */

#pragma once

#include <metaprogramming.h>

/**
 *  <summary>Determines whether two blocks of memory are equal or not.</summary>
 *  <seealso cref="memcmp"/>
 *  <param name="src1">Start of the first memory block.</param>
 *  <param name="src2">Start of the second memory block.</param>
 *  <param name="len">The size of the memory blocks, in bytes.</param>
 *  <return>True if the memory blocks contain the same values; otherwise false.</return>
 */
__extern __used bool memeq(void const * src1, void const * src2, size_t len);

/**
 *  <summary>Compares the given blocks of memory.</summary>
 *  <remarks>When testing for equality, <see cref="memeq"/> is preferred.</remarks>
 *  <seealso cref="memeq"/>
 *  <param name="src1">Start of the first memory block.</param>
 *  <param name="src2">Start of the second memory block.</param>
 *  <param name="len">The size of the memory blocks, in bytes.</param>
 *  <return>
 *  Less than 0 if first block is lesser than the second, greater than 0 if the first block is
 *  greater than the second, or 0 if the blocks are equal.
 *  </return>
 */
__extern __used comp_t memcmp(void const * src1, void const * src2, size_t len);

/**
 *  <summary>
 *  Searches for the first occurrence of <paramref name="val"/> in the given block of memory.
 *  </summary>
 *  <param name="src">Start of the memory block.</param>
 *  <param name="val">Value to look for; it is compared as a byte, not an integer.</param>
 *  <param name="len">The size of the memory block, in bytes.</param>
 *  <return>
 *  A pointer to the first occurrence of <paramref name="val"/> in the given block of memory if
 *  found; otherwise <c>nullptr</c>.
 *  </return>
 */
__extern __used void * memchr(void const * src, int val, size_t len);

/**
 *  <summary>
 *  Copies <paramref name="len"/>bytes from a source memory block to a destination memory block.
 *  </summary>
 *  <remarks>
 *  When the source and destination regions may overlap, <see cref="memmove"/> should be used
 *  instead.
 *  </remarks>
 *  <seealso cref="memmove"/>
 *  <param name="dst">Start of the destionation memory block.</param>
 *  <param name="src">Start of the source memory block.</param>
 *  <param name="len">The number of bytes to copy from the source to the destination.</param>
 *  <return><paramref name="dst"/> is returned.</return>
 */
__extern __used void * memcpy(void * dst, void const * src, size_t len);

/**
 *  <summary>
 *  Copies <paramref name="len"/>bytes from a source memory block to a destination memory block.
 *  </summary>
 *  <remarks>
 *  When the source and destination regions are guarenteed to be distinct (will never overlap),
 *  <see cref="memcpy"/> should be used instead.
 *  </remarks>
 *  <seealso cref="memcpy"/>
 *  <param name="dst">Start of the destionation memory block.</param>
 *  <param name="src">Start of the source memory block.</param>
 *  <param name="len">The number of bytes to copy from the source to the destination.</param>
 *  <return><paramref name="dst"/> is returned.</return>
 */
__extern __used void * memmove(void * dst, void const * src, size_t len);

/**
 *  <summary>
 *  Sets all the bytes in the given block of memory to <paramref name="val"/>.
 *  </summary>
 *  <param name="src">Start of the memory block.</param>
 *  <param name="val">Value to set the bytes to; it is treated as a byte, not an integer.</param>
 *  <param name="len">The size of the memory block, in bytes.</param>
 *  <return>
 *  A pointer to the first occurrence of <paramref name="val"/> in the given block of memory if
 *  found; otherwise <c>nullptr</c>.
 *  </return>
 */
__extern __used void * memset(void * dst, int const val, size_t len);

/**
 *  <summary>Obtains the length (in characters) of a C string.</summary>
 *  <param name="str">Start of C string.</param>
 *  <return>The length of the string.</return>
 */
__extern __used size_t strlen(char const * str);

/**
 *  <summary>Obtains the length (in characters) of a C string with an upper bound.</summary>
 *  <param name="str">Start of C string.</param>
 *  <param name="len">Maximum number of characters to measure.</param>
 *  <return>The length of the string bound by <paramref name="len"/>.</return>
 */
__extern __used size_t strnlen(char const * str, size_t len);

/**
 *  <summary>
 *  Obtains the length (in characters) of a C string with an upper bound and reports whether the
 *  actual terminator was reached or not.</summary>
 *  <param name="str">Start of C string.</param>
 *  <param name="len">Maximum number of characters to measure.</param>
 *  <param name="reached">
 *  Upon returning, will be set to <c>true</c> if the actual null-terminator was reached;
 *  otherwise <c>false</c>.
 *  </param>
 *  <return>The length of the string bound by <paramref name="len"/>.</return>
 */
__extern __used size_t strnlenex(char const * str, size_t len, bool * reached);

__extern __used comp_t strcmp(char const * src1, char const * src2);
__extern __used comp_t strncmp(char const * src1, char const * src2, size_t len);

__extern __used comp_t strcasecmp(char const * src1, char const * src2);
__extern __used comp_t strcasencmp(char const * src1, char const * src2, size_t len);

__extern __used char const * strstr(char const * haystack, char const * needle);

__extern __used char const * strstrex(char const * haystack, char const * needle, char const * seps);
__extern __used char const * strcasestrex(char const * haystack, char const * needle, char const * seps);
