#pragma once

#include <handles.h>

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
    };
    //  GCC makes this 8 bytes on AMD64.

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

        __bland inline ObjectPoolBase()
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

        __bland inline bool Contains(const uintptr_t object, const size_t objectSize, const size_t headerSize) const
        {
            uintptr_t start = ((uintptr_t)this) + headerSize;
            return object >= start && object <= (start + (this->Capacity - 1) * objectSize);
        }

        __bland inline bool Contains(const uintptr_t object, obj_ind_t & ind, const size_t objectSize, const size_t headerSize) const
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

        __bland inline obj_ind_t IndexOf(const uintptr_t object, const size_t objectSize, const size_t headerSize) const
        {
            return (obj_ind_t)((object - headerSize - ((uintptr_t)this)) / objectSize);
        }

        __bland inline FreeObject * GetFirstFreeObject(const size_t objectSize, const size_t headerSize) const
        {
            return (FreeObject *)(uintptr_t)((uintptr_t)this + headerSize + this->FirstFreeObject * objectSize);
        }
        __bland inline FreeObject * GetLastFreeObject(const size_t objectSize, const size_t headerSize) const
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
}}
