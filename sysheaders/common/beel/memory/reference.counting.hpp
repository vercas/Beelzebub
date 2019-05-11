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
#include <new>

namespace Beelzebub { namespace Memory {
    template<typename T>
    struct LocalPointer;

    template<typename T>
    struct GlobalPointer;

    extern void ReportDanglingUniquePointer(void * value);

    namespace {
        template<typename T> struct __RemoveReference       { typedef T Type; };
        template<typename T> struct __RemoveReference<T &>  { typedef T Type; };
        template<typename T> struct __RemoveReference<T &&> { typedef T Type; };

        template<typename T> inline typename __RemoveReference<T>::Type && Move(T && arg) { return (typename __RemoveReference<T>::Type &&)arg; }
    }

    template<typename T>
    struct UniquePointer
    {
        friend struct LocalPointer<T>;
        friend struct GlobalPointer<T>;

        /*  Constructors  */

        inline UniquePointer(T * ptr) : Pointer( ptr) { }

        inline UniquePointer(UniquePointer<T> && other)
            : Pointer(Move(other.Pointer))
        {
            other.Pointer = nullptr;
        }

        inline UniquePointer<T> & operator =(UniquePointer<T> && other)
        {
            this->Pointer = Move(other.Pointer);
            other.Pointer = nullptr;

            return *this;
        }

        UniquePointer(UniquePointer const &) = delete;
        UniquePointer & operator =(UniquePointer const &) = delete;

        /*  Destructor  */

        inline ~UniquePointer()
        {
            if unlikely(this->Pointer != nullptr)
                ReportDanglingUniquePointer(this->Pointer);
        }

        /*  Accessing  */

        T * operator->() const { return   this->Pointer;  }
        T & operator*()  const { return *(this->Pointer); }

        explicit operator bool() const { return this->Pointer != nullptr; }

        T * Get() const { return this->Pointer; }

        T * Release()
        {
            T * tmp = this->Pointer;
            this->Pointer = nullptr;
            return tmp;
        }

    private:
        /*  Fields  */

        T * Pointer;
    };

    template<typename T>
    class ReferenceCounted
    {
        friend struct LocalPointer<T>::LocalCopy;
        friend struct GlobalPointer<T>;

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
        ReferenceCounted(ReferenceCounted &&) = delete;
        ReferenceCounted & operator =(ReferenceCounted const &) = delete;
        ReferenceCounted & operator =(ReferenceCounted &&) = delete;

    protected:
        /*  Reference Counting  */

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
    struct GlobalPointer
    {
        friend struct LocalPointer<T>;
        friend struct LocalPointer<T>::LocalCopy;

        /*  Constructors  */

        inline GlobalPointer() : Pointer( nullptr) { }

        inline GlobalPointer(GlobalPointer<T> const & other)
            : Pointer( other.Pointer)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->AcquireReference();
        }

        inline GlobalPointer(GlobalPointer<T> && other)
            : Pointer(Move(other.Pointer))
        {
            other.Pointer = nullptr;
        }

        inline GlobalPointer(LocalPointer<T> ptr)
        {
            if likely(ptr.Pointer != nullptr)
                (this->Pointer = ptr.Pointer->Pointer)->AcquireReference();
            else
                this->Pointer = nullptr;
        }

        inline GlobalPointer<T> & operator =(GlobalPointer<T> const & other)
        {
            if unlikely(this->Pointer == other.Pointer)
                return *this;

            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();

            if likely((this->Pointer = other.Pointer) != nullptr)
                this->Pointer->AcquireReference();

            return *this;
        }

        inline GlobalPointer<T> & operator =(GlobalPointer<T> && other)
        {
            if unlikely(this->Pointer == Move(other.Pointer))
            {
                other.Pointer = nullptr;

                return *this;
            }

            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();

            this->Pointer = other.Pointer;
            other.Pointer = nullptr;

            return *this;
        }

        inline GlobalPointer<T> & operator =(LocalPointer<T> other)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();

            if likely(other.Pointer != nullptr)
                (this->Pointer = other.Pointer->Pointer)->AcquireReference();

            return *this;
        }

        /*  Destructor  */

        inline ~GlobalPointer()
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();
        }

        /*  Accessing  */

        inline __must_check T * Get() const { return static_cast<T *>(this->Pointer); }

        inline T * operator->() const { return  this->Get(); }
        inline T & operator*()  const { return *this->Get(); }

        explicit inline operator bool() const { return this->Pointer != nullptr; }

        /*  Comparisons  */

        inline bool operator ==(T * other) const { return this->Pointer == other; }
        inline bool operator !=(T * other) const { return this->Pointer != other; }

        inline bool operator ==(GlobalPointer<T> const & other) const { return this->Pointer == other.Pointer; }
        inline bool operator !=(GlobalPointer<T> const & other) const { return this->Pointer != other.Pointer; }

    private:
        /*  Fields  */

        ReferenceCounted<T> * Pointer;
    };

    template<typename T>
    struct LocalPointer
    {
        friend struct GlobalPointer<T>;

    private:
        /*  Subtypes  */

        struct LocalCopy
        {
            friend struct GlobalPointer<T>;
            friend struct LocalPointer<T>;

        public:
            /*  Constructors  */

            inline LocalCopy(ReferenceCounted<T> * ptr)
                : Count( 1)
                , Pointer(ptr)
            {
                ptr->AcquireReference();
            }

            inline LocalCopy(ReferenceCounted<T> * ptr, bool increaseCount)
                : Count( 1)
                , Pointer(ptr)
            {
                if unlikely(increaseCount)
                    ptr->AcquireReference();
                //  Unlikely because the other overload would be used.
            }

            LocalCopy(LocalCopy const &) = delete;
            LocalCopy & operator =(LocalCopy const &) = delete;

            /*  Reference Counting  */

            inline size_t AcquireReference() { return this->Count++; };

            inline size_t ReleaseReference()
            {
                size_t count;

                if unlikely((count = --this->Count) == 0)
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

        inline constexpr LocalPointer() : Pointer( nullptr) { }

        inline LocalPointer(LocalPointer<T> const & ptr)
            : Pointer( ptr.Pointer)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->AcquireReference();
        }

        inline LocalPointer(LocalPointer<T> && other)
            : Pointer(Move(other.Pointer))
        {
            other.Pointer = nullptr;
        }

        // inline LocalPointer(GlobalPointer<T> ptr)
        // {
        //     if likely(ptr.Pointer != nullptr)
        //         this->Pointer = new LocalCopy(ptr.Pointer);
        // }

        // inline LocalPointer(UniquePointer<T> const & other)
        // {
        //     if likely(other.Pointer != nullptr)
        //         this->Pointer = new LocalCopy(other.Release());
        //     else
        //         this->Pointer = nullptr;
        // }

        inline LocalPointer(UniquePointer<T> && other)
        {
            if likely(other.Pointer != nullptr)
                this->Pointer = new LocalCopy(other.Release());
            else
                this->Pointer = nullptr;
        }

        inline LocalPointer(GlobalPointer<T> const & other)
        {
            if likely(other.Pointer != nullptr)
                this->Pointer = new LocalCopy(other.Pointer);
            else
                this->Pointer = nullptr;
        }

        inline LocalPointer(GlobalPointer<T> && other)
        {
            if likely(other.Pointer != nullptr)
            {
                this->Pointer = new LocalCopy(other.Pointer, false);

                other.Pointer = nullptr;
            }
            else
                this->Pointer = nullptr;
        }

        inline LocalPointer(ReferenceCounted<T> * ptr)
        {
            if likely(ptr != nullptr)
                this->Pointer = new LocalCopy(ptr);
            else
                this->Pointer = nullptr;
        }

        inline LocalPointer & operator =(LocalPointer<T> const & other)
        {
            if unlikely(this->Pointer == other.Pointer)
                return *this;

            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();

            if likely((this->Pointer = other.Pointer) != nullptr)
                this->Pointer->AcquireReference();

            return *this;
        }

        inline LocalPointer & operator =(LocalPointer<T> && other)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();

            this->Pointer = Move(other.Pointer);
            other.Pointer = nullptr;

            return *this;
        }

        // inline LocalPointer & operator =(UniquePointer<T> const & other)
        // {
        //     if likely(this->Pointer != nullptr)
        //         this->Pointer->ReleaseReference();

        //     if likely((this->Pointer = other.Pointer) != nullptr)
        //         this->Pointer->AcquireReference();

        //     return *this;
        // }

        inline LocalPointer & operator =(UniquePointer<T> && other)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();

            if likely(other.Pointer != nullptr)
                this->Pointer = new LocalCopy(other.Release());
            else
                this->Pointer = nullptr;

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

        inline LocalPointer & operator =(ReferenceCounted<T> * other)
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();

            if likely(other != nullptr)
                this->Pointer = new LocalCopy(other);

            return *this;
        }

        /*  Destructor  */

        inline ~LocalPointer()
        {
            if likely(this->Pointer != nullptr)
                this->Pointer->ReleaseReference();
        }

        /*  Accessing  */

        __must_check T * Get() const { return this->Pointer ? static_cast<T *>(this->Pointer->Pointer) : nullptr; }

        T * operator->() const { return  this->Get(); }
        T & operator*()  const { return *this->Get(); }

        explicit operator bool() const { return this->Pointer != nullptr; }

        /*  Comparisons  */

        inline bool operator ==(T * other) const { return this->Pointer == nullptr ? (other == nullptr) : this->Pointer->Pointer == other; }
        inline bool operator !=(T * other) const { return this->Pointer == nullptr ? (other != nullptr) : this->Pointer->Pointer != other; }

        inline bool operator ==(LocalPointer<T> const & other) const
        {
            return this->Pointer == other.Pointer || (this->Pointer != nullptr && other.Pointer != nullptr && this->Pointer->Pointer == other.Pointer->Pointer);
            //  Either they point to the same copies, or they point to copies that point to the same object.
        }
        inline bool operator !=(LocalPointer<T> const & other) const { return !this->operator ==(other); }

        inline bool operator ==(GlobalPointer<T> const & other) const { return this->Pointer == nullptr ? (other.Pointer == nullptr) : this->Pointer->Pointer == other.Pointer; }
        inline bool operator !=(GlobalPointer<T> const & other) const { return this->Pointer == nullptr ? (other.Pointer != nullptr) : this->Pointer->Pointer != other.Pointer; }

    private:
        /*  Fields  */

        LocalCopy * Pointer;
    };
}}
