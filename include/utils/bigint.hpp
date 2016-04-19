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

#include <math.h>

namespace Beelzebub { namespace Utils
{
    __shared bool BigIntAdd(uint32_t * dst, uint32_t const * src1, uint32_t const * src2, uint32_t size, bool cin);
    __shared bool BigIntSub(uint32_t * dst, uint32_t const * src1, uint32_t const * src2, uint32_t size, bool cin);

    __shared bool BigIntMul(uint32_t       * dst , uint32_t & dstSize
                          , uint32_t const * src1, uint32_t   size1
                          , uint32_t const * src2, uint32_t   size2
                          , uint32_t maxSize, bool cin);

    __shared void BigIntDiv(uint32_t       * quot, uint32_t sizeQ
                          , uint32_t       * remn, uint32_t sizeR
                          , uint32_t const * src1, uint32_t size1
                          , uint32_t const * src2, uint32_t size2);

    __shared void BigIntAnd(uint32_t       * dst , uint32_t sizeD
                          , uint32_t const * src1, uint32_t size1
                          , uint32_t const * src2, uint32_t size2);

    __shared void BigIntOr (uint32_t       * dst , uint32_t sizeD
                          , uint32_t const * src1, uint32_t size1
                          , uint32_t const * src2, uint32_t size2);

    __shared void BigIntXor(uint32_t       * dst , uint32_t sizeD
                          , uint32_t const * src1, uint32_t size1
                          , uint32_t const * src2, uint32_t size2);

    __shared void BigIntNot(uint32_t       * dst , uint32_t sizeD
                          , uint32_t const * src , uint32_t sizeS);

    __shared bool BigIntShL(uint32_t       * dst , uint32_t & sizeD
                          , uint32_t const * src , uint32_t   sizeS
                          , uint32_t sizeM, uint64_t amnt);

    __shared bool BigIntShR(uint32_t       * dst , uint32_t & sizeD
                          , uint32_t const * src , uint32_t   sizeS
                          , uint64_t amnt);

    __shared int  BigIntCmp(uint32_t const * srcL, uint32_t sizeL
                          , uint32_t const * srcR, uint32_t sizeR);

    __shared bool BigIntGetPow10(uint32_t * dst, uint32_t & sizeD
                               , uint32_t sizeM, uint32_t exponent);

    template<uint32_t MaxSize>
    struct BigUInt
    {
        /*  Operations  */

        static inline uint32_t Balance(BigUInt & left, BigUInt & right)
        {
            if (left.CurrentSize > right.CurrentSize)
            {
                for (size_t i = right.CurrentSize; i < left.CurrentSize; ++i)
                    right.Data[i] = 0;

                return left.CurrentSize;
            }
            else
            {
                for (size_t i = left.CurrentSize; i < right.CurrentSize; ++i)
                    left.Data[i] = 0;

                return right.CurrentSize;
            }
        }

        /*  Constructors  */

        inline BigUInt() : CurrentSize(0) { }

        inline BigUInt(uint64_t const val) : CurrentSize((MaxSize > 1) ? 2 : 1)
        {
            this->Data[0] = (uint32_t)(val & 0xFFFFFFFFU);

            if (MaxSize > 1)
                this->Data[1] = (uint32_t)(val >> 32);

            // this->Data[0] = (uint32_t)(val & 0xFFFFFFFFU);

            // if (MaxSize > 1)
            // {
            //     if (val > 0xFFFFFFFFU)
            //     {
            //         this->CurrentSize = 2U;
            //         this->Data[1] = (uint32_t)(val >> 32);
            //     }
            //     else if (val > 0)
            //         this->CurrentSize = 1U;
            //     else
            //         this->CurrentSize = 0U;
            // }
            // else
            //     this->CurrentSize = (this->Data[0] == 0) ? 0U : 1U;
        }

        inline BigUInt(BigUInt const & other) : CurrentSize(other.CurrentSize)
        {
            for (size_t i = 0; i < this->CurrentSize; ++i)
                this->Data[i] = other.Data[i];
        }

        inline BigUInt(BigUInt const && other) : CurrentSize(other.CurrentSize)
        {
            for (size_t i = 0; i < this->CurrentSize; ++i)
                this->Data[i] = other.Data[i];
        }

        inline BigUInt & operator =(BigUInt const & other)
        {
            this->CurrentSize = other.CurrentSize;

            for (size_t i = 0; i < this->CurrentSize; ++i)
                this->Data[i] = other.Data[i];
        }

        inline BigUInt & operator =(BigUInt const && other)
        {
            this->CurrentSize = other.CurrentSize;

            for (size_t i = 0; i < this->CurrentSize; ++i)
                this->Data[i] = other.Data[i];

            return *this;
        }

        static inline BigUInt GetPowerOf10(uint32_t exponent)
        {
            BigUInt res {};

            BigIntGetPow10(&(res.Data[0]), res.CurrentSize, MaxSize, exponent);

            return res;
        }

        /*  Conversions  */

        inline uint32_t ToUInt32() const
        {
            if unlikely(this->CurrentSize == 0)
                return 0U;
            else
                return this->Data[0];
        }

        inline explicit operator uint32_t() const
        {
            return this->ToUInt32();
        }

        inline uint64_t ToUInt64() const
        {
            if unlikely(this->CurrentSize == 0)
                return 0U;
            else if (this->CurrentSize == 1)
                return (uint64_t)(this->Data[0]);
            else
                return (uint64_t)(this->Data[0]) | ((uint64_t)(this->Data[1]) << 32);
        }

        inline explicit operator uint64_t() const
        {
            return this->ToUInt64();
        }

        /*  Arithmetic Operators  */

        inline BigUInt operator +(BigUInt & other)
        {
            BigUInt res {};

            bool cout = BigIntAdd(&(res.Data[0]), &(this->Data[0]), &(other.Data[0])
                , res.CurrentSize = Balance(*this, other), false);

            if unlikely(cout && res.CurrentSize < MaxSize)
                res.Data[res.CurrentSize++] = 1U;

            return res;
        }

        inline BigUInt operator +(BigUInt && other)
        {
            BigUInt res {};

            bool cout = BigIntAdd(&(res.Data[0]), &(this->Data[0]), &(other.Data[0])
                , res.CurrentSize = Balance(*this, other), false);

            if unlikely(cout && res.CurrentSize < MaxSize)
                res.Data[res.CurrentSize++] = 1U;

            return res;
        }

        inline BigUInt & operator +=(BigUInt & other)
        {
            bool cout = BigIntAdd(&(this->Data[0]), &(this->Data[0]), &(other.Data[0])
                , Balance(*this, other), false);

            if unlikely(cout && this->CurrentSize < MaxSize)
                this->Data[this->CurrentSize++] = 1U;

            return *this;
        }

        inline BigUInt & operator +=(BigUInt && other)
        {
            bool cout = BigIntAdd(&(this->Data[0]), &(this->Data[0]), &(other.Data[0])
                , Balance(*this, other), false);

            if unlikely(cout && this->CurrentSize < MaxSize)
                this->Data[this->CurrentSize++] = 1U;

            return *this;
        }

        inline BigUInt operator -(BigUInt & other)
        {
            BigUInt res {};

            BigIntSub(&(res.Data[0]), &(this->Data[0]), &(other.Data[0])
                , res.CurrentSize = Balance(*this, other), false);

            return res;
        }

        inline BigUInt operator -(BigUInt && other)
        {
            BigUInt res {};

            BigIntSub(&(res.Data[0]), &(this->Data[0]), &(other.Data[0])
                , res.CurrentSize = Balance(*this, other), false);

            return res;
        }

        inline BigUInt & operator -=(BigUInt & other)
        {
            BigIntSub(&(this->Data[0]), &(this->Data[0]), &(other.Data[0])
                , Balance(*this, other), false);

            return *this;
        }

        inline BigUInt & operator -=(BigUInt && other)
        {
            BigIntSub(&(this->Data[0]), &(this->Data[0]), &(other.Data[0])
                , Balance(*this, other), false);

            return *this;
        }

        inline BigUInt operator *(BigUInt & other) const
        {
            BigUInt res {};

            BigIntMul(&(  res.Data[0]),   res.CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize
                    , MaxSize, false);

            return res;
        }

        inline BigUInt operator *(BigUInt && other) const
        {
            BigUInt res {};

            BigIntMul(&(  res.Data[0]),   res.CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize
                    , MaxSize, false);

            return res;
        }

        inline BigUInt & operator *=(BigUInt & other)
        {
            BigIntMul(&(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize
                    , MaxSize, false);

            return *this;
        }

        inline BigUInt & operator *=(BigUInt && other)
        {
            BigIntMul(&(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize
                    , MaxSize, false);

            return *this;
        }

        inline BigUInt operator /(BigUInt & other) const
        {
            BigUInt res {};

            res.CurrentSize = Maximum(this->CurrentSize, other.CurrentSize);

            BigIntDiv(&(  res.Data[0]),   res.CurrentSize
                    , nullptr         , 0
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return res;
        }

        inline BigUInt operator /(BigUInt && other) const
        {
            BigUInt res {};

            res.CurrentSize = Maximum(this->CurrentSize, other.CurrentSize);

            BigIntDiv(&(  res.Data[0]),   res.CurrentSize
                    , nullptr         , 0
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return res;
        }

        inline BigUInt & operator /=(BigUInt & other)
        {
            BigIntDiv(&(this->Data[0]), this->CurrentSize
                    , nullptr         , 0
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return *this;
        }

        inline BigUInt & operator /=(BigUInt && other)
        {
            BigIntDiv(&(this->Data[0]), this->CurrentSize
                    , nullptr         , 0
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return *this;
        }

        inline BigUInt operator %(BigUInt & other) const
        {
            BigUInt res {};

            res.CurrentSize = Maximum(this->CurrentSize, other.CurrentSize);

            BigIntDiv(nullptr         , 0
                    , &(  res.Data[0]),   res.CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return res;
        }

        inline BigUInt operator %(BigUInt && other) const
        {
            BigUInt res {};

            res.CurrentSize = Maximum(this->CurrentSize, other.CurrentSize);

            BigIntDiv(nullptr         , 0
                    , &(  res.Data[0]),   res.CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return res;
        }

        inline BigUInt & operator %=(BigUInt & other)
        {
            BigIntDiv(nullptr         , 0
                    , &(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return *this;
        }

        inline BigUInt & operator %=(BigUInt && other)
        {
            BigIntDiv(nullptr         , 0
                    , &(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return *this;
        }

        /*  Logical Operators  */

        inline BigUInt operator &(BigUInt const & other) const
        {
            BigUInt res {};

            BigIntAnd(&(  res.Data[0]),   res.CurrentSize = Minimum(this->CurrentSize, other.CurrentSize)
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            //  Minimum is chosen because the surplus would be all zeros anyway.

            return res;
        }

        inline BigUInt operator &(BigUInt && other) const
        {
            BigUInt res {};

            BigIntAnd(&(  res.Data[0]),   res.CurrentSize = Minimum(this->CurrentSize, other.CurrentSize)
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return res;
        }

        inline BigUInt & operator &=(BigUInt const & other)
        {
            BigIntAnd(&(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return *this;
        }

        inline BigUInt & operator &=(BigUInt && other)
        {
            BigIntAnd(&(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return *this;
        }

        inline BigUInt operator |(BigUInt const & other) const
        {
            BigUInt res {};

            BigIntOr (&(  res.Data[0]),   res.CurrentSize = Maximum(this->CurrentSize, other.CurrentSize)
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            //  Maximum is chosen because surplus comes from the largest source.

            return res;
        }

        inline BigUInt operator |(BigUInt && other) const
        {
            BigUInt res {};

            BigIntOr (&(  res.Data[0]),   res.CurrentSize = Maximum(this->CurrentSize, other.CurrentSize)
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return res;
        }

        inline BigUInt & operator |=(BigUInt const & other)
        {
            BigIntOr (&(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return *this;
        }

        inline BigUInt & operator |=(BigUInt && other)
        {
            BigIntOr (&(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return *this;
        }

        inline BigUInt operator ^(BigUInt const & other) const
        {
            BigUInt res {};

            BigIntXor(&(  res.Data[0]),   res.CurrentSize = Maximum(this->CurrentSize, other.CurrentSize)
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            //  Maximum is chosen because surplus comes from the largest source.

            return res;
        }

        inline BigUInt operator ^(BigUInt && other) const
        {
            BigUInt res {};

            BigIntXor(&(  res.Data[0]),   res.CurrentSize = Maximum(this->CurrentSize, other.CurrentSize)
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return res;
        }

        inline BigUInt & operator ^=(BigUInt const & other)
        {
            BigIntXor(&(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return *this;
        }

        inline BigUInt & operator ^=(BigUInt && other)
        {
            BigIntXor(&(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , &(other.Data[0]), other.CurrentSize);

            return *this;
        }

        inline BigUInt operator ~() const
        {
            BigUInt res {};

            BigIntNot(&(  res.Data[0]),   res.CurrentSize = this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize);

            return res;
        }

        inline BigUInt operator <<(uint64_t const amnt) const
        {
            BigUInt res {};

            BigIntShL(&(  res.Data[0]),   res.CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , MaxSize, amnt);

            return res;
        }

        inline BigUInt operator <<(BigUInt const & other) const
        {
            return *this << (uint64_t)other;
        }

        inline BigUInt operator <<(BigUInt && other) const
        {
            return *this << (uint64_t)other;
        }

        inline BigUInt & operator <<=(uint64_t const amnt)
        {
            BigIntShL(&(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , MaxSize, amnt);

            return *this;
        }

        inline BigUInt & operator <<=(BigUInt const & other)
        {
            return *this <<= (uint64_t)other;
        }

        inline BigUInt & operator <<=(BigUInt && other) const
        {
            return *this <<= (uint64_t)other;
        }

        inline BigUInt operator >>(uint64_t const amnt) const
        {
            BigUInt res {};

            BigIntShR(&(  res.Data[0]),   res.CurrentSize = this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , amnt);

            return res;
        }

        inline BigUInt operator >>(BigUInt const & other) const
        {
            return *this >> other.ToUInt64();
        }

        inline BigUInt operator >>(BigUInt && other) const
        {
            return *this >> other.ToUInt64();
        }

        inline BigUInt & operator >>=(uint64_t const amnt)
        {
            BigIntShR(&(this->Data[0]), this->CurrentSize
                    , &(this->Data[0]), this->CurrentSize
                    , amnt);

            return *this;
        }

        inline BigUInt & operator >>=(BigUInt const & other)
        {
            return *this >>= other.ToUInt64();
        }

        inline BigUInt & operator >>=(BigUInt && other)
        {
            return *this >>= other.ToUInt64();
        }

        /*  Comparison Operators  */

        inline bool operator ==(BigUInt const & other) const
        {
            return 0 == BigIntCmp(&(this->Data[0]), this->CurrentSize
                                , &(other.Data[0]), other.CurrentSize);
        }

        inline bool operator ==(BigUInt && other) const
        {
            return 0 == BigIntCmp(&(this->Data[0]), this->CurrentSize
                                , &(other.Data[0]), other.CurrentSize);
        }

        inline bool operator !=(BigUInt const & other) const
        {
            return !(this->operator ==(other));
        }

        inline bool operator !=(BigUInt && other) const
        {
            return !(this->operator ==(other));
        }

        inline bool operator >(BigUInt const & other) const
        {
            return 0 < BigIntCmp(&(this->Data[0]), this->CurrentSize
                               , &(other.Data[0]), other.CurrentSize);
        }

        inline bool operator >(BigUInt && other) const
        {
            return 0 < BigIntCmp(&(this->Data[0]), this->CurrentSize
                               , &(other.Data[0]), other.CurrentSize);
        }

        inline bool operator <=(BigUInt const & other) const
        {
            return !(this->operator >(other));
        }

        inline bool operator <=(BigUInt && other) const
        {
            return !(this->operator >(other));
        }

        inline bool operator >=(BigUInt const & other) const
        {
            return 0 <= BigIntCmp(&(this->Data[0]), this->CurrentSize
                                , &(other.Data[0]), other.CurrentSize);
        }

        inline bool operator >=(BigUInt && other) const
        {
            return 0 <= BigIntCmp(&(this->Data[0]), this->CurrentSize
                                , &(other.Data[0]), other.CurrentSize);
        }

        inline bool operator <(BigUInt const & other) const
        {
            return !(this->operator >=(other));
        }

        inline bool operator <(BigUInt && other) const
        {
            return !(this->operator >=(other));
        }

        /*  Slices  */

        inline BigUInt operator [](uint32_t size) const
        {
            BigUInt res {};
            res.CurrentSize = Minimum(size, MaxSize);

            if (size <= this->CurrentSize)
                for (size_t i = 0; i < size; ++i)
                    res.Data[i] = this->Data[i];
            else
            {
                for (size_t i = 0; i < this->CurrentSize; ++i)
                    res.Data[i] = this->Data[i];

                for (size_t i = this->CurrentSize; i < size; ++i)
                    res.Data[i] = 0;
                //  Zero-extension.
            }

            return res;
        }

        /*  Fields  */

        uint32_t CurrentSize;
        uint32_t Data[MaxSize];
        //  Looks like a length-prefixed array.

    } __aligned(16);
}}
