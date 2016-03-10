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

#include <utils/bigint.hpp>
#include <string.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

union Qword
{
    uint64_t u64;
    int64_t  i64;

    struct
    {
        uint32_t u32l;
        uint32_t u32h;
    };

    struct
    {
        int32_t i32l;
        int32_t i32h;
    };
};

/*  Arithmetic Operations  */

bool Beelzebub::Utils::BigIntMul(uint32_t       * dst , uint32_t & dstSize
                               , uint32_t const * src1, uint32_t   size1
                               , uint32_t const * src2, uint32_t   size2
                               , uint32_t maxSize, bool cin)
{
    if (dst == src1)
    {
        uint32_t bck[size1];  //  A backup.

        memcpy(&(bck[0]), src1, size1 * sizeof(uint32_t));
        //  Meh.

        return BigIntMul(dst, dstSize, &(bck[0]), size1, src2, size2, maxSize, cin);
    }
    else if (dst == src2)
    {
        uint32_t bck[size2];  //  A backup.

        memcpy(&(bck[0]), src2, size2 * sizeof(uint32_t));
        //  Meh.

        return BigIntMul(dst, dstSize, src1, size1, &(bck[0]), size2, maxSize, cin);
    }

    uint32_t const limit = dstSize = Minimum(size1 + size2, maxSize);
    //  Number of dwords in the destination.

    // if (dst == src1)
    //     return BigIntMul2(dst, size1, src2, size2, limit, cin);
    // else if (dst == src2)
    //     return BigIntMul2(dst, size2, src1, size1, limit, cin);

    if (size1 > size2)
    {
        uint32_t sizeTmp = size1;
        size1 = size2;
        size2 = sizeTmp;

        uint32_t const * srcTmp = src1;
        src1 = src2;
        src2 = srcTmp;
    }

    //  Now `src1` is the shortest number.

    memset(&(dst[0]), 0, limit * sizeof(uint32_t));

    dst[0] = cin ? 1U : 0U;
    //  First dword is the carry in.

    Qword res;  //  Will be the last multiplication result.
    bool overflow = false;

    for (size_t i = 0; i < size1; ++i)
    {
        uint32_t currentDword = src1[i];

        if (currentDword != 0U)  //  Cheap optimization, kek.
            for (size_t j = 0, k = i; j < size2 && k < limit; ++j, ++k)
            {
                res = {(uint64_t)currentDword * (uint64_t)src2[j]};

                for (size_t l = k; res.u64 != 0 && l < limit; ++l, res.u64 >>= 32)
                {
                    res.u64 += (uint64_t)dst[l];
                    //  Done this way to catch carry into the overflow.

                    dst[l] = res.u32l;
                }

                if (res.u32h != 0)
                    overflow = true;
            }
    }

    return overflow;
}

void Beelzebub::Utils::BigIntDiv(uint32_t       * quot, uint32_t sizeQ
                               , uint32_t       * remn, uint32_t sizeR
                               , uint32_t const * src1, uint32_t size1
                               , uint32_t const * src2, uint32_t size2)
{
    if (quot == nullptr)
    {
        sizeQ = Maximum(size1, size2);

        uint32_t bck[sizeQ];
        memset(&(bck[0]), 0, sizeQ * sizeof(uint32_t));

        BigIntDiv(&(bck[0]), sizeQ, remn, sizeR, src1, size1, src2, size2);

        return;
    }
    else if (remn == nullptr)
    {
        sizeR = Maximum(size1, size2);

        uint32_t bck[sizeR];
        memset(&(bck[0]), 0, sizeR * sizeof(uint32_t));

        BigIntDiv(quot, sizeQ, &(bck[0]), sizeR, src1, size1, src2, size2);

        return;
    }
    else if (src1 == quot || src1 == remn)
    {
        uint32_t bck[size1];
        memcpy(&(bck[0]), src1, size1 * sizeof(uint32_t));

        BigIntDiv(quot, sizeQ, remn, sizeR, &(bck[0]), size1, src2, size2);

        return;
    }
    else if (src2 == quot || src2 == remn)
    {
        uint32_t bck[size2];
        memcpy(&(bck[0]), src2, size2 * sizeof(uint32_t));

        BigIntDiv(quot, sizeQ, remn, sizeR, src1, size1, &(bck[0]), size2);

        return;
    }

    //  Isn't that a rather large number of special cases?
}

/*  Logic Operations  */

__noinline void BigIntAnd2(uint32_t       * dst, uint32_t sizeD
                         , uint32_t const * src, uint32_t sizeS)
{
    if (sizeS > sizeD)
        sizeS = sizeD;
    //  There is nothing to do with the surplus.

    for (size_t i = 0; i < sizeS; ++i)
        dst[i] &= src[i];

    if (sizeD > sizeS)
        memset(dst + sizeS, 0, (sizeD - sizeS) * sizeof(uint32_t));
        //  The rest becomes 0, as if ANDed with 0.
}

void Beelzebub::Utils::BigIntAnd(uint32_t       * dst , uint32_t sizeD
                               , uint32_t const * src1, uint32_t size1
                               , uint32_t const * src2, uint32_t size2)
{
    if (dst == src1)
        BigIntAnd2(dst, Minimum(sizeD, size1), src2, size2);
    else if (dst == src2)
        BigIntAnd2(dst, Minimum(sizeD, size2), src1, size1);
    else
    {
        uint32_t const limit = Minimum(size1, size2);

        for (size_t i = 0; i < limit; ++i)
            dst[i] = src1[i] & src2[i];

        if (sizeD > limit)
            memset(dst + limit, 0, (sizeD - limit) * sizeof(uint32_t));
        //  The rest becomes 0.
    }
}

__noinline void BigIntOr2(uint32_t       * dst, uint32_t sizeD
                         , uint32_t const * src, uint32_t sizeS)
{
    if (sizeS > sizeD)
        sizeS = sizeD;
    //  There is nothing to do with the surplus.

    for (size_t i = 0; i < sizeS; ++i)
        dst[i] |= src[i];

    //  The rest remains unchanged, as if ORed with 0.
}

void Beelzebub::Utils::BigIntOr (uint32_t       * dst , uint32_t sizeD
                               , uint32_t const * src1, uint32_t size1
                               , uint32_t const * src2, uint32_t size2)
{
    if (dst == src1)
        BigIntOr2(dst, Minimum(sizeD, size1), src2, size2);
    else if (dst == src2)
        BigIntOr2(dst, Minimum(sizeD, size2), src1, size1);
    else
    {
        if (size1 > sizeD)
            size1 = sizeD;
        
        if (size2 > sizeD)
            size2 = sizeD;

        //  There is nothing to do with the surpluses.

        uint32_t limit = Minimum(size1, size2);

        size_t i = 0;

        for (/* nothing */; i < limit; ++i)
            dst[i] = src1[i] | src2[i];

        if (size1 > limit)
        {
            for (/* nothing */; i < size1; ++i)
                dst[i] = src1[i];

            limit = size1;
        }
        else if (size2 > limit)
        {
            for (/* nothing */; i < size2; ++i)
                dst[i] = src2[i];

            limit = size2;
        }

        //  The shortest source is zero-extended (virtually) to the same size as
        //  the longest (other) source.

        if (sizeD > limit)
            memset(dst + limit, 0, (sizeD - limit) * sizeof(uint32_t));
        //  The rest becomes 0.
    }
}

__noinline void BigIntXor2(uint32_t       * dst, uint32_t sizeD
                         , uint32_t const * src, uint32_t sizeS)
{
    if (sizeS > sizeD)
        sizeS = sizeD;
    //  There is nothing to do with the surplus.

    for (size_t i = 0; i < sizeS; ++i)
        dst[i] ^= src[i];

    //  The rest remains unchanged, as if XORed with 0.
}

void Beelzebub::Utils::BigIntXor(uint32_t       * dst , uint32_t sizeD
                               , uint32_t const * src1, uint32_t size1
                               , uint32_t const * src2, uint32_t size2)
{
    if (dst == src1)
        BigIntXor2(dst, Minimum(sizeD, size1), src2, size2);
    else if (dst == src2)
        BigIntXor2(dst, Minimum(sizeD, size2), src1, size1);
    else
    {
        if (size1 > sizeD)
            size1 = sizeD;
        
        if (size2 > sizeD)
            size2 = sizeD;

        //  There is nothing to do with the surpluses.

        uint32_t limit = Minimum(size1, size2);

        size_t i = 0;

        for (/* nothing */; i < limit; ++i)
            dst[i] = src1[i] ^ src2[i];

        if (size1 > limit)
        {
            for (/* nothing */; i < size1; ++i)
                dst[i] = src1[i];

            limit = size1;
        }
        else if (size2 > limit)
        {
            for (/* nothing */; i < size2; ++i)
                dst[i] = src2[i];

            limit = size2;
        }

        //  The shortest source is zero-extended (virtually) to the same size as
        //  the longest (other) source.

        if (sizeD > limit)
            memset(dst + limit, 0, (sizeD - limit) * sizeof(uint32_t));
        //  The rest becomes 0.
    }
}

void Beelzebub::Utils::BigIntNot(uint32_t       * dst, uint32_t sizeD
                               , uint32_t const * src, uint32_t sizeS)
{
    uint32_t const limit = Minimum(sizeD, sizeS);

    for (size_t i = 0; i < limit; ++i)
        dst[i] = ~src[i];

    //  `i` stands for "innuendo".
}

bool Beelzebub::Utils::BigIntShL(uint32_t       * dst, uint32_t & sizeD
                               , uint32_t const * src, uint32_t   sizeS
                               , uint32_t sizeM, uint64_t amnt)
{
    size_t const amntMov = (size_t)(amnt >> 5);
    //  This is the amount of array positions to shift left by.

    amnt &= 31;
    //  This will be the actual amount to shift by.

    if (amntMov >= sizeM)
    {
        //  This will shift more positions than available.

        memset(dst, 0, (sizeD = sizeM) * sizeof(uint32_t));
        //  Means all that's left is zeros.

        return true;
    }
    else
    {
        sizeD = Minimum(sizeM, sizeS + amntMov);

        uint32_t overflow = 0U;

        if (amntMov != 0)
        {
            for (size_t i = sizeD - 1; i >= amntMov; --i)
                dst[i] = src[i - amntMov];

            //  This is done in reverse so that nothing is lost when the source
            //  and destination are the same.

            for (size_t i = amntMov; i < sizeD; ++i)
            {
                Qword window { (uint64_t)(src[i]) };
                //  This should 0-extended the value.

                window.u64 = (window.u64 << amnt) | (uint64_t)overflow;

                dst[i] = window.u32l;
                overflow = window.u32h;
            }

            //  This has to be done in order, because overflow propagates towards
            //  more significant bits/dwords.

            memset(dst, 0, amntMov * sizeof(uint32_t));

            //  No carry in accounted for.
        }
        else
            for (size_t i = 0; i < sizeD; ++i)
            {
                Qword window { (uint64_t)(src[i]) };
                //  This should 0-extended the value.

                window.u64 = (window.u64 << amnt) | (uint64_t)overflow;

                dst[i] = window.u32l;
                overflow = window.u32h;
            }

        if (overflow != 0U)
        {
            if (sizeD < sizeM)
                dst[sizeD++] = overflow;    //  Maybe there's room for overflow.
            else
                return true;
        }

        return false;
    }
}

bool Beelzebub::Utils::BigIntShR(uint32_t       * dst, uint32_t & sizeD
                               , uint32_t const * src, uint32_t   sizeS
                               , uint64_t amnt)
{
    if unlikely(src == dst)
    {
        uint32_t bck[sizeS];
        memcpy(&(bck[0]), src, sizeS * sizeof(uint32_t));

        return BigIntShR(dst, sizeD, &(bck[0]), sizeS, amnt);
    }

    size_t amntMov = (size_t)(amnt >> 5);
    //  This is the amount of array positions to shift right by.

    amnt = 32 - (amnt & 31);
    //  This will be the actual amount to shift LEFT by.
    //  A dword will be shifted left 32 positions, then right (amnt & 31)
    //  positions, which is reduced to a single left shift.

    if (amntMov >= sizeS)
    {
        //  This will shift more positions than available.

        sizeD = 0;  //  Nothing else should be necessary.

        return true;
    }
    else
    {
        uint32_t underflow = (sizeS > sizeD) ? (src[sizeD] >> (32 - amnt)) : 0U;
        //  There may be underflow already.

        if (amntMov != 0)
        {
            sizeD = Minimum(sizeS, sizeD) - amntMov;

            --amntMov;
            //  Makes the following index computation easier.

            for (size_t i = sizeD; i > 0; --i)
            {
                Qword window { ((uint64_t)(src[i + amntMov]) << amnt) };

                window.u32h |= (uint64_t)underflow;

                dst[i - 1] = window.u32h;
                underflow = window.u32l;
            }

            //  This has to be done in reverse order, because underflow
            //  propagates towards less significant bits/dwords.
        }
        else
        {
            if (sizeS < sizeD)
                sizeD = sizeS;

            for (size_t i = sizeD; i > 0; --i)
            {
                Qword window { ((uint64_t)(src[i - 1]) << amnt) };

                window.u32h |= (uint64_t)underflow;

                dst[i - 1] = window.u32h;
                underflow = window.u32l;
            }
        }

        while (dst[sizeD - 1] == 0) --sizeD;    //  Trim excess zeros.

        return underflow != 0U;
    }
}

/*  Comparisons  */

int Beelzebub::Utils::BigIntCmp(uint32_t const * srcL, uint32_t sizeL
                              , uint32_t const * srcR, uint32_t sizeR)
{
    if (sizeL > sizeR)
    {
        for (size_t i = sizeR; i < sizeL; ++i)
            if (srcL[i] != 0)
                return 1;

        //  Since the rest of the right source is assumed to be 0, any non-zero
        //  element in the left side's extra means it is larger.

        sizeL = sizeR;
    }
    else if (sizeR > sizeL)
    {
        for (size_t i = sizeL; i < sizeR; ++i)
            if (srcR[i] != 0)
                return -1;

        //  Analogy for the right source.
        //  And no need to update `sizeR`.
    }

    //  Sizes are equal now.

    if (srcL == srcR || sizeL == 0)
        return 0;
    //  Just in case. Also, this check is done here (after the sizes) in case
    //  the caller tries to do something "smart".

    size_t i = sizeL;

    do
    {
        --i;

        uint32_t const l = srcL[i], r = srcR[i];

        if (l > r)
            return 1;
        else if (r > l)
            return -1;
    } while (i > 0);

    return 0;
}
