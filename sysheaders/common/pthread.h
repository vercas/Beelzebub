/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum PTHREAD_MUTEX
{
    PTHREAD_MUTEX_DEFAULT,
    PTHREAD_MUTEX_NORMAL,
    PTHREAD_MUTEX_ERRORCHECK,
    PTHREAD_MUTEX_RECURSIVE,
};

typedef struct pthread_mutexattr_s
{
    enum PTHREAD_MUTEX Type;
} pthread_mutexattr_t;

#ifdef __cplusplus
}
#endif

#ifdef __BEELZEBUB_KERNEL

    #include <beel/sync/smp.lock.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pthread_mutex_s
    {
        SmpLockUni Lock;
        SmpLockUniCookie Cookie;
    } pthread_mutex_t;

    #define PTHREAD_MUTEX_INITIALIZER {{0}, {NULL}}

    static inline void pthread_mutex_lock(pthread_mutex_t * const m)
    {
        SmpLockUniCookie const ck = SmpLockUniAcquire(&(m->Lock));

        m->Cookie = ck;
    }

    static inline void pthread_mutex_unlock(pthread_mutex_t * const m)
    {
        SmpLockUniRelease(&(m->Lock), m->Cookie);
    }

    static inline int pthread_mutex_init(pthread_mutex_t * __restrict const m
                                        , pthread_mutexattr_t const * __restrict const attr)
    {
        (void)attr;
        
        SmpLockUniReset(&(m->Lock));
    }

    static inline int pthread_mutexattr_init(pthread_mutexattr_t * attr)
    {
        (void)attr;

        return 0;
    }

    static inline int pthread_mutexattr_destroy(pthread_mutexattr_t * attr)
    {
        (void)attr;

        return 0;
    }

    static inline int pthread_mutexattr_gettype(pthread_mutexattr_t const * attr, int * type)
    {
        *type = attr->Type;

        return 0;
    }

    static inline int pthread_mutexattr_settype(pthread_mutexattr_t * attr, int type)
    {
        switch (type)
        {
        case PTHREAD_MUTEX_ERRORCHECK:
        case PTHREAD_MUTEX_RECURSIVE:
            return 1;
        }

        attr->Type = (enum PTHREAD_MUTEX)type;

        return 0;
    }

    #ifdef __cplusplus
    }
    #endif

#endif
