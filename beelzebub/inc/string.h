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
__extern __used       bool   memeq  (const void *       src1, const void *       src2,       size_t len);

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
__extern __used       comp_t memcmp (const void *       src1, const void *       src2,       size_t len);

#ifdef __cplusplus
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
  extern __used const void * memchr (const void *       src , const int          val ,       size_t len);

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
  extern __used       void * memchr (      void *       src , const int          val ,       size_t len);
#else   //  C vs C++. :(

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
__extern __used       void * memchr (const void * const src , const int          val ,       size_t len);
#endif

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
__extern __used       void * memcpy (      void *       dst , const void *       src ,       size_t len);

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
__extern __used       void * memmove(      void *       dst , const void *       src ,       size_t len);

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
__extern __used       void * memset (      void *       dst , const int          val ,       size_t len);

/**
 *  <summary>Obtains the length (in characters) of a C string.</summary>
 *  <param name="str">Start of C string.</param>
 *  <return>The length of the string.</return>
 */
__extern __used       size_t strlen (const char *       str                                             );

/**
 *  <summary>Obtains the length (in characters) of a C string with an upper bound.</summary>
 *  <param name="str">Start of C string.</param>
 *  <param name="len">Maximum number of characters to measure.</param>
 *  <return>The length of the string bound by <paramref name="len"/>.</return>
 */
__extern __used       size_t strnlen  (const char *       str                          ,        size_t len                );

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
__extern __used       size_t strnlenex(const char *       str                          ,        size_t len, bool * reached);

__extern __used       comp_t strcmp (const char *       src1, const char *       src2                   );

__extern __used       comp_t strncmp(const char *       src1, const char *       src2,        size_t len);
