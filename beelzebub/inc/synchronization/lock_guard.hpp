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

namespace Beelzebub { namespace Synchronization
{
    /// <summary>Guards a scope under a lock.</summary>
    template<typename TLock, typename TCook>
    struct LockGuard
    {
        /*  Constructor(s)  */

        inline LockGuard(TLock & lock)
            : Lock(&lock)
            , Cookie(lock.Acquire())
        {
            
        }

        inline LockGuard(LockGuard && other)
            : Lock(other.Lock)
            , Cookie(other.Cookie)
        {
            other.Lock = nullptr;

            //  The other lock guard would be destructed after this.
            //  It need not unlock.
        }

        LockGuard(LockGuard const &) = delete;
        LockGuard & operator =(LockGuard const &) = delete;
        LockGuard & operator =(LockGuard &&) = delete;

        /*  Destructor  */

        inline ~LockGuard()
        {
            if (this->Lock != nullptr)
                this->Lock->Release(this->Cookie);
        }

        /*  Operations  */

        inline bool Release()
        {
            if (this->Lock != nullptr)
            {
                this->Lock->Release(this->Cookie);

                this->Lock = nullptr;

                return true;
            }

            return false;
        }

        inline bool Swap(TLock & other)
        {
            if (this->Lock != nullptr)
            {
                other.SimplyAcquire();
                this->Lock->SimplyRelease();
                //  The other lock is acquired before the current is released.

                this->Lock = &other;

                return true;
            }
            else
            {
                this->Cookie = (this->Lock = &other)->Acquire();

                return false;
            }
        }

    private:
        /*  Field(s)  */

        TLock * Lock;
        TCook Cookie;
    };

    /// <summary>Guards a scope under a lock.</summary>
    template<typename TLock>
    struct LockGuard<TLock, void>
    {
        /*  Constructor(s)  */

        inline LockGuard(TLock & lock) : Lock(&lock)
        {
            lock.Acquire();
        }

        inline LockGuard(LockGuard && other)
            : Lock(other.Lock)
        {
            other.Lock = nullptr;
            
            //  The other lock guard would be destructed after this.
            //  It need not unlock.
        }

        LockGuard(LockGuard const &) = delete;
        LockGuard & operator =(LockGuard const &) = delete;
        LockGuard & operator =(LockGuard &&) = delete;

        /*  Destructor  */

        inline ~LockGuard()
        {
            if (this->Lock != nullptr)
                this->Lock->Release();
        }

        /*  Operations  */

        inline bool Release()
        {
            if (this->Lock != nullptr)
            {
                this->Lock->Release();

                this->Lock = nullptr;

                return true;
            }

            return false;
        }

        inline bool Swap(TLock & other)
        {
            if (this->Lock != nullptr)
            {
                other.Acquire();
                this->Lock->Release();
                //  The other lock is acquired before the current is released.

                this->Lock = &other;

                return true;
            }
            else
            {
                (this->Lock = &other)->Acquire();

                return false;
            }
        }

    private:
        /*  Field(s)  */

        TLock * Lock;
    };

    template<typename TLock, typename TCook = typename TLock::Cookie>
    inline LockGuard<TLock, TCook> ObtainLockGuard(TLock & lock, TCook * blergh = nullptr)
    {
        return {lock};
    }

    /*template<typename TLock>
    inline LockGuardSimple<TLock> ObtainLockGuard<TLock, void>(TLock & lock)
    {
        return LockGuardSimple<TLock>(lock);
    }//*/
}}
