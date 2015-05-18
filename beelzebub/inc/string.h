/**
 *  Yeah, there are C++-specific declarations too.
 *  The C and common ones are defined in 'string.c'.
 *  The C++-specific ones are defined in 'string.cpp'.
 */

#pragma once

#include <metaprogramming.h>

__extern       bool   memeq  (const void * const src1, const void * const src2, size_t len);
__extern       comp_t memcmp (const void * const src1, const void * const src2, size_t len);
#ifdef __cplusplus
  extern const void * memchr (const void * const src , const int          val , size_t len);
  extern       void * memchr (      void * const src , const int          val , size_t len);
#else   //  C vs C++. :(
__extern       void * memchr (const void * const src , const int          val , size_t len);
#endif
__extern       void * memcpy (      void * const dst , const void * const src , size_t len);
__extern       void * memmove(      void * const dst , const void * const src , size_t len);
__extern       void * memset (      void * const dst , const int          val , size_t len);
