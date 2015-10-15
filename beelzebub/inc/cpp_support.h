#pragma once

#include "stdint.h"
#include "stddef.h"

#ifdef __cplusplus
    inline void * operator new     (size_t, void * p) { return p; }
    inline void * operator new[]   (size_t, void * p) { return p; }
    inline void   operator delete  (void *, void *  ) { }
    inline void   operator delete[](void *, void *  ) { }
#endif
    