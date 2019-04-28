/*
    Copyright (c) 2019 Alexandru-Mihai Maftei. All rights reserved.


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

#include <beel/metaprogramming.h>

namespace Beelzebub { namespace Memory {
    template<typename T>
    struct LocalPointer;

    template<typename T>
    struct GlobalPointer;

    template<typename T>
    class ReferenceCounted
    {
        friend class GlobalPointer<T>;
        friend class LocalPointer<T>::LocalCopy;

    public:
        /*  Constructors  */

        inline ReferenceCounted()
            : AcquireCount( 0)
            , AcquirePadding()
            , ReleaseCount( 0)
            , ReleasePadding()
        {

        }

        ReferenceCounted(ReferenceCounted const &) = delete;
        ReferenceCounted & operator =(ReferenceCounted const &) = delete;

    private:
        /*  Methods  */

        inline size_t AcquireReference() { return __atomic_fetch_add(&(this->AcquireCount), 1, __ATOMIC_ACQUIRE); };

        inline size_t ReleaseReference()
        {
            size_t count = __atomic_add_fetch(&(this->ReleaseCount), 1, __ATOMIC_ACQUIRE);

            if unlikely(count == __atomic_load_n(&(this->AcquireCount), __ATOMIC_RELAXED))
            {
                ((T *)this)->ReleaseMemory();
            }

            return count;
        };

        /*  Fields  */

        size_t AcquireCount;
        uint8_t AcquirePadding[64 - sizeof(size_t)]; //  TODO: Non-magic-constant cache line size? (64)
        size_t ReleaseCount;
        uint8_t ReleasePadding[64 - sizeof(size_t)]; //  TODO: Non-magic-constant cache line size? (64)
    };

    template<typename T>
    class GlobalPointer
    {
        friend class LocalPointer<T>;
        friend class LocalPointer<T>::LocalCopy;

    public:
        /*  Constructors  */

        inline GlobalPointer() : Pointer( nullptr) { }

        inline GlobalPointer(GlobalPointer * ptr)
            : Pointer( ptr->Pointer)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->AcquireReference();
        }

        inline GlobalPointer(GlobalPointer const & other)
        {
            if likely((this->Pointer = other.Pointer) != nullptr)
                this->Pointer->AcquireReference();
        }

        inline GlobalPointer & operator =(GlobalPointer const & other)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();
            if likely((this->Pointer = other.Pointer) != nullptr)
                this->Pointer->AcquireReference();
            return *this;
        }

        /*  Destructor  */

        inline ~GlobalPointer()
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();
        }

    private:
        /*  Fields  */

        ReferenceCounted<T> * Pointer;
    };

    template<typename T>
    struct LocalPointer
    {
    private:
        /*  Subtypes  */

        class LocalCopy
        {
        public:
            /*  Constructors  */

            inline LocalCopy(ReferenceCounted<T> * ptr)
                : Count( 1)
                , Pointer(ptr)
            {
                ptr->AcquireReference();
            }

            LocalCopy(LocalCopy const &) = delete;
            LocalCopy & operator =(LocalCopy const &) = delete;

            /*  Methods  */

            inline size_t AcquireReference() { return this->Count++; };

            inline size_t ReleaseReference()
            {
                size_t count;

                if unlikely((count == --this->Count) == 0)
                    delete this;    //  Commit sudoku.

                return count;
            };

        private:
            /*  Fields  */

            size_t Count;
            ReferenceCounted<T> * Pointer;
        };

    public:
        /*  Constructors  */

        inline LocalPointer() : Pointer( nullptr) { }

        inline LocalPointer(LocalPointer * ptr)
            : Pointer( ptr->Pointer)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->AcquireReference();
        }

        inline LocalPointer(GlobalPointer<T> * ptr)
        {
            if likely(ptr->Pointer != nullptr)
                this->Pointer = new LocalCopy(ptr->Pointer);
        }

        inline LocalPointer(LocalPointer const & other)
        {
            if likely((this->Pointer = other.Pointer) != nullptr)
                this->Pointer->AcquireReference();
        }

        inline LocalPointer(GlobalPointer<T> const & other)
        {
            if likely(other.Pointer != nullptr)
                this->Pointer = new LocalCopy(other.Pointer);
        }

        inline LocalPointer & operator =(LocalPointer const & other)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();
            if likely((this->Pointer = other.Pointer) != nullptr)
                this->Pointer->AcquireReference();
            return *this;
        }

        inline LocalPointer & operator =(GlobalPointer<T> const & other)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();
            if likely(other.Pointer != nullptr)
                this->Pointer = new LocalCopy(other.Pointer);
            return *this;
        }

        /*  Destructor  */

        inline ~LocalPointer()
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();
        }

    private:
        /*  Fields  */

        LocalCopy * Pointer;
    };
}}
