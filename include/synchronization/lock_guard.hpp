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

#pragma once

#include <metaprogramming.h>

#define ObtainLockGuard(name, lock) Beelzebub::Synchronization::LockGuard<decltype(lock)> name {lock}
#define withLock(lock) with(ObtainLockGuard(MCATS(_lock_guard_, __LINE__), lock))

#define ObtainNullLockGuardFlexible(name, lock) Beelzebub::Synchronization::LockGuardFlexible<decltype(lock)> name
#define ObtainLockGuardFlexible(name, lock) Beelzebub::Synchronization::LockGuardFlexible<decltype(lock)> name {lock}
#define withLockFlexible(lock) with(ObtainLockGuardFlexible(MCATS(_lock_guard_flexible_, __LINE__), lock))

namespace Beelzebub { namespace Synchronization
{
    /// <summary>Guards a scope under a lock.</summary>
    template<typename TLock, typename TCook = typename TLock::Cookie>
    struct LockGuard
    {
        /*  Constructor(s)  */

        __forceinline LockGuard(TLock & lock)
            : Lock(&lock)
            , Cookie(lock.Acquire())
        {
            
        }

        LockGuard(LockGuard const &) = delete;
        LockGuard(LockGuard && other) = delete;
        LockGuard & operator =(LockGuard const &) = delete;
        LockGuard & operator =(LockGuard &&) = delete;

        /*  Destructor  */

        __forceinline ~LockGuard()
        {
            this->Lock->Release(this->Cookie);
        }

    private:
        /*  Field(s)  */

        TLock * const Lock;
        TCook const Cookie;
    };

    /// <summary>Guards a scope under a lock.</summary>
    template<typename TLock>
    struct LockGuard<TLock, void>
    {
        /*  Constructor(s)  */

        __forceinline LockGuard(TLock & lock) : Lock(&lock)
        {
            lock.Acquire();
        }

        LockGuard(LockGuard const &) = delete;
        LockGuard(LockGuard && other) = delete;
        LockGuard & operator =(LockGuard const &) = delete;
        LockGuard & operator =(LockGuard &&) = delete;

        /*  Destructor  */

        __forceinline ~LockGuard()
        {
            this->Lock->Release();
        }

    private:
        /*  Field(s)  */

        TLock * const Lock;
    };

    /**************************************
        And now, a flexible lock guard!
    **************************************/

    /// <summary>Guards a scope under a lock.</summary>
    template<typename TLock, typename TCook = typename TLock::Cookie>
    struct LockGuardFlexible
    {
        /*  Constructor(s)  */

        __forceinline LockGuardFlexible()
            : Lock(nullptr)
            , Cookie(int_cookie_invalid)
        {
            
        }

        __forceinline LockGuardFlexible(TLock & lock)
            : Lock(&lock)
            , Cookie((&lock == nullptr) ? int_cookie_invalid : lock.Acquire())
        {
            
        }

        LockGuardFlexible(LockGuardFlexible const &) = delete;
        LockGuardFlexible(LockGuardFlexible && other) = delete;
        LockGuardFlexible & operator =(LockGuardFlexible const &) = delete;
        LockGuardFlexible & operator =(LockGuardFlexible &&) = delete;

        /*  Destructor  */

        __forceinline ~LockGuardFlexible()
        {
            if (this->Lock != nullptr)
                this->Lock->Release(this->Cookie);
        }

        /*  Operations  */

        __forceinline void Swap(TLock & other)
        {
            if (this->Lock == nullptr)
                this->Cookie = (this->Lock = &other)->Acquire();
            else
            {
                other.SimplyAcquire();
                this->Lock->SimplyRelease();

                this->Lock = &other;
            }
        }

    private:
        /*  Field(s)  */

        TLock * Lock;
        TCook Cookie;
    };

    /// <summary>Guards a scope under a lock.</summary>
    template<typename TLock>
    struct LockGuardFlexible<TLock, void>
    {
        /*  Constructor(s)  */

        __forceinline LockGuardFlexible() : Lock(nullptr) { }

        __forceinline LockGuardFlexible(TLock & lock) : Lock(&lock)
        {
            if (&lock != nullptr)
                lock.Acquire();
        }

        LockGuardFlexible(LockGuardFlexible const &) = delete;
        LockGuardFlexible(LockGuardFlexible && other) = delete;
        LockGuardFlexible & operator =(LockGuardFlexible const &) = delete;
        LockGuardFlexible & operator =(LockGuardFlexible &&) = delete;

        /*  Destructor  */

        __forceinline ~LockGuardFlexible()
        {
            if (this->Lock != nullptr)
                this->Lock->Release();
        }

        /*  Operations  */

        __forceinline void Swap(TLock & other)
        {
            if (this->Lock == nullptr)
                (this->Lock = &other)->Acquire();
            else
            {
                other.Acquire();
                this->Lock->Release();
            }
        }

    private:
        /*  Field(s)  */

        TLock * Lock;
    };
}}
