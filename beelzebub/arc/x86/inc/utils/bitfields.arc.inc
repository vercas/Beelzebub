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

//  This file is for inclusion as text.

/*  Bit properties!!!1!  */

#ifdef __GCC_ASM_FLAG_OUTPUTS__
    #ifndef BITFIELD_FLAG_RO
    //  Creates a getter for bit-based properties.
    #define BITFIELD_FLAG_RO(bitInd, name, varT, var, decB, decG, decV)        \
    decV varT const MCATS(name, Bit) = ((varT)1) << bitInd;                    \
    decB inline int MCATS2(Get, name)() decG                                   \
    {                                                                          \
        int res;                                                               \
                                                                               \
        asm ( "bt %[bit], %[src] \n\t"                                         \
            : "=@ccc"(res) /* carry flag  */                                   \
            : [src]"rm"(var)                                                   \
            , [bit]"Jr"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }
    #endif

    #ifndef BITFIELD_FLAG_RW
    //  Creates a getter and setter for bit-based properties.
    #define BITFIELD_FLAG_RW(bitInd, name, varT, var, decB, decG, decV)        \
    decV varT const MCATS(name, Bit) = ((varT)1) << bitInd;                    \
    decB inline int MCATS(Get, name)() decG                                    \
    {                                                                          \
        int res;                                                               \
                                                                               \
        asm ( "bt %[bit], %[src] \n\t"                                         \
            : "=@ccc"(res) /* carry flag  */                                   \
            : [src]"rm"(var)                                                   \
            , [bit]"Jr"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline auto MCATS(Set, name)(bool const val)                          \
    {                                                                          \
        if (val)                                                               \
            asm ( "bts %[bit], %[dst] \n\t"                                    \
                : [dst]"+rm"(var)                                              \
                : [bit]"Jr"(bitInd)                                            \
                : "cc" );                                                      \
        else                                                                   \
            asm ( "btr %[bit], %[dst] \n\t"                                    \
                : [dst]"+rm"(var)                                              \
                : [bit]"Jr"(bitInd)                                            \
                : "cc" );                                                      \
                                                                               \
        return *this;                                                          \
    }                                                                          \
    decB inline int MCATS(FetchSet, name)()                                    \
    {                                                                          \
        int res;                                                               \
                                                                               \
        asm ( "bts %[bit], %[dst] \n\t"                                        \
            : [dst]"+rm"(var), "=@ccc"(res)                                    \
            : [bit]"Jr"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline int MCATS(FetchClear, name)()                                  \
    {                                                                          \
        int res;                                                               \
                                                                               \
        asm ( "btr %[bit], %[dst] \n\t"                                        \
            : [dst]"+rm"(var), "=@ccc"(res)                                    \
            : [bit]"Jr"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline int MCATS(FetchFlip, name)()                                   \
    {                                                                          \
        int res;                                                               \
                                                                               \
        asm ( "btc %[bit], %[dst] \n\t"                                        \
            : [dst]"+rm"(var), "=@ccc"(res)                                    \
            : [bit]"Jr"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }
    #endif
#else
    #ifndef BITFIELD_FLAG_RO
    //  Creates a getter for bit-based properties.
    #define BITFIELD_FLAG_RO(bitInd, name, varT, var, decB, decG, decV)        \
    decV varT const MCATS(name, Bit) = ((varT)1) << bitInd;                    \
    decB inline bool MCATS2(Get, name)() decG                                  \
    {                                                                          \
        bool res;                                                              \
                                                                               \
        asm("bt    %[bit], %[src] \n\t"                                        \
            "setcb %[res]         \n\t"                                        \
            : [res]"=qm"(res) /* carry flag  */                                \
            : [src]"rm"(var)                                                   \
            , [bit]"Jr"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }
    #endif

    #ifndef BITFIELD_FLAG_RW
    //  Creates a getter and setter for bit-based properties.
    #define BITFIELD_FLAG_RW(bitInd, name, varT, var, decB, decG, decV)        \
    decV varT const MCATS(name, Bit) = ((varT)1) << bitInd;                    \
    decB inline bool MCATS(Get, name)() decG                                   \
    {                                                                          \
        bool res;                                                              \
                                                                               \
        asm("bt    %[bit], %[src] \n\t"                                        \
            "setcb %[res]         \n\t"                                        \
            : [res]"=qm"(res)                                                  \
            : [src]"rm"(var)                                                   \
            , [bit]"Jr"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline auto MCATS(Set, name)(bool const val)                          \
    {                                                                          \
        if (val)                                                               \
            asm("bts   %[bit], %[dst] \n\t"                                    \
                : [dst]"+rm"(var)                                              \
                : [bit]"Jr"(bitInd)                                            \
                : "cc" );                                                      \
        else                                                                   \
            asm("btr   %[bit], %[dst] \n\t"                                    \
                : [dst]"+rm"(var)                                              \
                : [bit]"Jr"(bitInd)                                            \
                : "cc" );                                                      \
                                                                               \
        return *this;                                                          \
    }                                                                          \
    decB inline bool MCATS(FetchSet, name)()                                   \
    {                                                                          \
        bool res;                                                              \
                                                                               \
        asm("bts   %[bit], %[dst] \n\t"                                        \
            "setcb %[res]         \n\t"                                        \
            : [dst]"+rm"(var)                                                  \
            , [res]"=qm"(res)                                                  \
            : [bit]"Jr"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline bool MCATS(FetchClear, name)()                                 \
    {                                                                          \
        bool res;                                                              \
                                                                               \
        asm("btr   %[bit], %[dst] \n\t"                                        \
            "setcb %[res]         \n\t"                                        \
            : [dst]"+rm"(var)                                                  \
            , [res]"=qm"(res)                                                  \
            : [bit]"Jr"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }                                                                          \
    decB inline bool MCATS(FetchFlip, name)()                                  \
    {                                                                          \
        bool res;                                                              \
                                                                               \
        asm("btc   %[bit], %[dst] \n\t"                                        \
            "setcb %[res]         \n\t"                                        \
            : [dst]"+rm"(var)                                                  \
            , [res]"=qm"(res)                                                  \
            : [bit]"Jr"(bitInd)                                                \
            : "cc" );                                                          \
                                                                               \
        return res;                                                            \
    }
    #endif
#endif
