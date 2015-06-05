#pragma once

#include <metaprogramming.h>

#ifdef __cplusplus
    inline void *operator new(size_t, void *p)     throw() { return p; }
    inline void *operator new[](size_t, void *p)   throw() { return p; }
    inline void  operator delete  (void *, void *) throw() { };
    inline void  operator delete[](void *, void *) throw() { };
#endif
    