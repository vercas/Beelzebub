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

#include <math.h>

#ifdef __GCC_ASM_FLAG_OUTPUTS__
#define ADC32(res, dst, src)            \
    asm ( "adcl %k[src], %k[dst] \n\t"  \
        : [dst]"+rm"(dst), "=@ccc"(res) \
        : [src]"r"(src) )
#else
#define ADC32(res, dst, src)                \
    asm ( "adcl %k[src], %k[dst] \n\t"      \
          "setcb %b[res] \n\t"              \
        : [dst]"+rm"(dst), [res]"=rm"(res)  \
        : [src]"r"(src) )
#endif

uint32_t Beelzebub::AddWithCarry32(uint32_t * dst, uint32_t src, uint32_t cin)
{
    uint32_t res = 0;

    asm ( "bt %[bit], %k[cin] \n\t"
          "adcl %k[src], %k[dst] \n\t"
#ifndef __GCC_ASM_FLAG_OUTPUTS__
          "setcb %b[res] \n\t"
#endif
        : [dst]"+m"(*dst)
#ifdef __GCC_ASM_FLAG_OUTPUTS__
        , "=@ccc"(res)
#else
        , [res]"=rm"(res)
#endif
        : [src]"r"(src)
        , [cin]"rm"(cin)
        , [bit]"Nr"(0)
        : "flags");

    return res;
}

uint32_t Beelzebub::AddWithCarry32_3(uint32_t * dst, uint32_t src1, uint32_t src2, uint32_t cin)
{
    uint32_t res = 0;

    asm ( "bt %[bit], %k[cin] \n\t"
          "adcl %k[src1], %k[dst] \n\t"
          "setcb %b[res] \n\t"  //  This is the intermediate carry.
          "addl %k[src2], %k[dst] \n\t"
          "setcb %b[res] \n\t"
        : [dst]"+m"(*dst)
        , [res]"=rm"(res)
        : [src1]"r"(src1)
        , [src2]"r"(src2)
        , [cin]"rm"(cin)
        , [bit]"Nr"(0)
        : "flags");

    return res;
}

#ifdef __BEELZEBUB__ARCH_AMD64
uint32_t Beelzebub::AddWithCarry64(uint64_t * dst, uint64_t src, uint32_t cin)
{
    uint32_t res = 0;

    asm ( "bt %[bit], %k[cin] \n\t"
          "adcq %q[src], %q[dst] \n\t"
#ifndef __GCC_ASM_FLAG_OUTPUTS__
          "setcb %b[res] \n\t"
#endif
        : [dst]"+m"(*dst)
#ifdef __GCC_ASM_FLAG_OUTPUTS__
        , "=@ccc"(res)
#else
        , [res]"=rm"(res)
#endif
        : [src]"r"(src)
        , [cin]"rm"(cin)
        , [bit]"Nr"(0)
        : "flags");

    return res;
}
#else
uint32_t Beelzebub::AddWithCarry64(uint64_t * dst, uint64_t src, uint32_t cin)
{
    uint32_t res = 0;

    asm ( "bt %[bit], %k[cin] \n\t"
          "adcl %k[src1], %k[dst1] \n\t"
          "adcl %k[src2], %k[dst2] \n\t"
#ifndef __GCC_ASM_FLAG_OUTPUTS__
          "setcb %b[res] \n\t"
#endif
        : [dst1]"+m"(*reinterpret_cast<uint32_t *>(dst    ))
        , [dst2]"+m"(*reinterpret_cast<uint32_t *>(dst + 1))
#ifdef __GCC_ASM_FLAG_OUTPUTS__
        , "=@ccc"(res)
#else
        , [res]"=rm"(res)
#endif
        : [src1]"r"((uint32_t)src)
        , [src2]"r"((uint32_t)(src >> 32))
        , [cin]"rm"(cin)
        , [bit]"Nr"(0)
        : "flags");

    return res;
}
#endif
