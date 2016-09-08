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

#include <beel/handles.h>

namespace Beelzebub { namespace Memory
{
    /** <summary>Type used to represent the index of an object.</summary> */
    typedef uint32_t obj_ind_t;

    /** <summary>A set of predefined object index values with special meanings.</summary> */
    enum obj_ind_special : obj_ind_t
    {
        obj_ind_invalid = 0xFFFFFFFFU,
    };

    /**
     *  <summary>Represents the contents of a free object in the object pool.</summary>
     */
    struct FreeObject
    {
        obj_ind_t Dummy;
        //  This is here to allow the free flag bit to reside in the lower 32 bits.

        obj_ind_t Next;
    } __packed;

    /**
     *  <summary>Represents an area of memory where objects can be allocated.</summary>
     */
    struct ObjectPoolBase
    {
        /*  Fields  */

        obj_ind_t volatile FreeCount;
        obj_ind_t Capacity;  //  This should help determine the size of the pool.
        obj_ind_t FirstFreeObject;      //  Used by the allocator.
        obj_ind_t LastFreeObject;       //  Used for enlarging, mainly.
        //  These should be creating an alignment of up to 16 if needed.

        ObjectPoolBase * Next;
        //ObjectPoolBase * Previous;

        /*  Constructors  */

        inline ObjectPoolBase()
            : FreeCount( 0)
            , Capacity(0)
            , FirstFreeObject(obj_ind_invalid)
            , LastFreeObject(obj_ind_invalid)
            , Next(nullptr)
        {

        }

        ObjectPoolBase(ObjectPoolBase const &) = delete;
        ObjectPoolBase & operator =(const ObjectPoolBase &) = delete;
        //  Aye - no copying.

        /*  Methods  */

        inline bool Contains(const uintptr_t object, const size_t objectSize, const size_t headerSize) const
        {
            uintptr_t start = ((uintptr_t)this) + headerSize;
            return object >= start && object <= (start + (this->Capacity - 1) * objectSize);
        }

        inline bool Contains(const uintptr_t object, obj_ind_t & ind, const size_t objectSize, const size_t headerSize) const
        {
            uintptr_t const start = ((uintptr_t)this) + headerSize;

            if likely(object >= start)
            {
                uintptr_t const offset = (object - start) / objectSize;

                if likely(offset < (uintptr_t)this->Capacity)
                {
                    ind = (obj_ind_t)offset;
                    return true;
                }
            }

            ind = obj_ind_invalid;
            return false;
        }

        inline obj_ind_t IndexOf(const uintptr_t object, const size_t objectSize, const size_t headerSize) const
        {
            return (obj_ind_t)((object - headerSize - ((uintptr_t)this)) / objectSize);
        }

        inline FreeObject * GetFirstFreeObject(const size_t objectSize, const size_t headerSize) const
        {
            return (FreeObject *)(uintptr_t)((uintptr_t)this + headerSize + this->FirstFreeObject * objectSize);
        }
        inline FreeObject * GetLastFreeObject(const size_t objectSize, const size_t headerSize) const
        {
            return (FreeObject *)(uintptr_t)((uintptr_t)this + headerSize + this->LastFreeObject * objectSize);
        }
    };

    typedef Handle (*AcquirePoolFunc)(size_t objectSize, size_t headerSize, size_t minimumObjects, ObjectPoolBase * & result);
    //  The acquiring code need not lock the object pool.
    typedef Handle (*EnlargePoolFunc)(size_t objectSize, size_t headerSize, size_t minimumExtraObjects, ObjectPoolBase * pool);
    //  The enlarging code will operate with a fully-locked pool and will leave it as such.
    typedef Handle (*ReleasePoolFunc)(size_t objectSize, size_t headerSize, ObjectPoolBase * pool);
    //  The release code need not (un)lock the object poool.

    //  Why so many parameters? So the provider can make the best decisions! :)

    /// <summary>Options for controlling the way empty pools are released</summary>
    enum class PoolReleaseOptions
    {
        /// <summary>All pools will be released when empty.</summary>
        ReleaseAll = 0,
        /// <summary>All pools will be released when empty, except for the last pool remaining.</summary>
        KeepOne = 1,
        /// <summary>No pools will be released when emptied.</summary>
        NoRelease = 2,
    };
}}
