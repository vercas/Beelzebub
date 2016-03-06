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

/*  The inline assembly in this file uses an arcane feature of GCC documented
    here: https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#x86Operandmodifiers

    Cheers to froggey on #osdev for the info and documentation.
*/

#include <utils/bigint.hpp>
#include <string.h>

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

// __noinline bool BigIntAdd2(uint32_t * dst, uint32_t const * src, uint32_t size, bool cin)
// {
//     uint32_t carry = cin ? 1U : 0U;

//     while (size > 1)
//     {
//         //  First, add all 64-bit pairs.

//         asm volatile ( "bt %[bit], %k[carry] \n\t"
//                        "adcq %[src], %[dst] \n\t"
//                        "setcb %b[carry] \n\t"
//                      : [dst]"+m"(*(reinterpret_cast<uint64_t *>(dst)))
//                      , [carry]"+rm"(carry)
//                      : [src]"r"(*(reinterpret_cast<uint64_t const *>(src)))
//                      , [bit]"Nr"(0)
//                      );

//         size -= 2;
//         dst += 2;
//         src += 2;
//     }

//     if (size == 1)
//     {
//         //  Then the possible remainder of 32 bits.

//         asm volatile ( "bt %[bit], %k[carry] \n\t"
//                        "adcl %[src], %[dst] \n\t"
//                        "setcb %b[carry] \n\t"
//                      : [dst]"+m"(*dst)
//                      , [carry]"+rm"(carry)
//                      : [src]"r"(*src)
//                      , [bit]"Nr"(0)
//                      );
//     }

//     return carry != 0U;
// }

// bool Beelzebub::Utils::BigIntAdd(uint32_t * dst
//     , uint32_t const * src1, uint32_t const * src2, uint32_t size, bool cin)
// {
//     if (dst == src1)
//         return BigIntAdd2(dst, src2, size, cin);
//     else if (dst == src2)
//         return BigIntAdd2(dst, src1, size, cin);

//     //  Otherwise, all 3 operands are different.

//     uint32_t carry = cin ? 1U : 0U;

//     while (size > 1)
//     {
//         //  First, add all 64-bit pairs.

//         uint64_t temp = *(reinterpret_cast<uint64_t const *>(src2));

//         asm ( "bt %[bit], %k[carry] \n\t"
//               "adcq %[src1], %[temp] \n\t"
//               "setcb %b[carry] \n\t"
//               "movq %[temp], %[dst] \n\t"
//             : [dst]"=m"(*(reinterpret_cast<uint64_t *>(dst)))
//             , [carry]"+rm"(carry)
//             : [src1]"m"(*(reinterpret_cast<uint64_t const *>(src1)))
//             , [temp]"r"(temp)
//             , [bit]"Nr"(0)
//             : "flags");

//         size -= 2;
//         dst += 2;
//         src1 += 2;
//         src2 += 2;
//     }

//     if (size == 1)
//     {
//         //  Then the possible remainder of 32 bits.

//         uint32_t temp = *src2;
        
//         asm ( "bt %[bit], %k[carry] \n\t"
//               "adcl %[src1], %[temp] \n\t"
//               "setcb %b[carry] \n\t"
//               "movl %[temp], %[dst] \n\t"
//             : [dst]"=m"(*dst)
//             , [carry]"+rm"(carry)
//             : [src1]"m"(*src1)
//             , [temp]"r"(temp)
//             , [bit]"Nr"(0)
//             : "flags");
//     }

//     return carry != 0U;
// }

__noinline bool BigIntMul2(uint32_t       * dst, uint32_t dstSize
                         , uint32_t const * src, uint32_t size
                         , uint32_t limit, bool cin)
{
    Qword res;  //  Will be the last multiplication result.

    uint32_t bck[dstSize];  //  A backup.

    memcpy(&(bck[0]), dst, dstSize * sizeof(uint32_t));
    //  Meh.

    for (size_t i = 0; i < dstSize; ++i)
    {
        uint32_t currentDword = bck[i];

        if (currentDword != 0U)  //  Cheap optimization, kek.
            for (size_t j = 0, k = i; j < size && k < limit; ++j, ++k)
            {
                res = {(uint64_t)currentDword * (uint64_t)src[j]};

                for (size_t l = k; res.u64 != 0 && l < limit; ++l, res.u64 >>= 32)
                {
                    res.u64 += (uint64_t)dst[l];
                    //  Done this way to catch carry into the overflow.

                    dst[l] = res.u32l;
                }
            }
    }

    return res.u32h != 0;
}

bool Beelzebub::Utils::BigIntMul(uint32_t       * dst , uint32_t & dstSize
                               , uint32_t const * src1, uint32_t   size1
                               , uint32_t const * src2, uint32_t   size2
                               , uint32_t maxSize, bool cin)
{
    uint32_t const limit = dstSize = Minimum(size1 + size2, maxSize);
    //  Number of dwords in the destination.

    if (dst == src1)
        return BigIntMul2(dst, size1, src2, size2, limit, cin);
    else if (dst == src2)
        return BigIntMul2(dst, size2, src1, size1, limit, cin);

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

    dst[0] = cin ? 1U : 0U;
    //  First dword is the carry out.

    for (size_t i = 1; i < limit; ++i)
        dst[i] = 0U;
    //  Initialize the rest to zero.

    Qword res;  //  Will be the last multiplication result.

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
            }
    }

    return res.u32h != 0;
}
