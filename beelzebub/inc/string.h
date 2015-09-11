/**
 *  Yeah, there are C++-specific declarations too.
 *  The C and common ones are defined in 'string.c'.
 *  The C++-specific ones are defined in 'string.cpp'.
 */

#pragma once

#include <metaprogramming.h>

__extern __used       bool   memeq  (const void * const src1, const void * const src2, size_t len);
__extern __used       comp_t memcmp (const void * const src1, const void * const src2, size_t len);
#ifdef __cplusplus
  extern __used const void * memchr (const void * const src , const int          val , size_t len);
  extern __used       void * memchr (      void * const src , const int          val , size_t len);
#else   //  C vs C++. :(
__extern __used       void * memchr (const void * const src , const int          val , size_t len);
#endif
__extern __used       void * memcpy (      void *       dst , const void *       src , size_t len);
__extern __used       void * memmove(      void *       dst , const void *       src , size_t len);
__extern __used       void * memset (      void *       dst , const int          val , size_t len);
